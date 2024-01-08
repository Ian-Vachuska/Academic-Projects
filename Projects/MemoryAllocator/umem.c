#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include "umem.h"

#define MAGIC 123456
typedef struct node {
    size_t size;
    struct node *next;
} node_t;
typedef struct {
    size_t size;
    int magic;
} header_t;

void *head = NULL;
void *freehead = NULL;
void *freenext = NULL;
void *freeprev = NULL;
int algoState = 0;

void *bestfit(size_t size);

void *worstfit(size_t size);

void *buddy(size_t size);

void *fnfit(void *_curr, size_t size);

int bfree(void *ptr);

void coalesce();

int umeminit(size_t sizeOfRegion, int allocationAlgo) {
    //validate size
    if (sizeOfRegion <= 0) { return -1; }
    //ensure umeminit is only called once
    if (algoState != 0) { return -1; }
    //validate algorithm
    if (allocationAlgo < 1 || 5 < allocationAlgo) { return -1; }
    if ((sizeOfRegion % getpagesize()) != 0) {
        sizeOfRegion += (getpagesize() - (sizeOfRegion % getpagesize()));
    }
    algoState = allocationAlgo;
    // open the /dev/zero device
    int fd = open("/dev/zero", O_RDWR);
    // sizeOfRegion (in bytes) needs to be evenly divisible by the page size
    head = mmap(NULL, sizeOfRegion + sizeof(node_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, fd, 0);//linux
    if (head == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    // close the device (don't worry, mapping should be unaffected)
    close(fd);
    freehead = head;
    node_t *node = (node_t *) head;
    node->size = sizeOfRegion - sizeof(node_t);
    node->next = NULL;
    //hide node at end mem for next
    freenext = head;// + sizeOfRegion - sizeof(node_t);
    if (algoState == BUDDY) { node->size += sizeof(node_t); }
    return 0;
}

void *umalloc(size_t size) {
    //round size up to be divisible by 8
    if (size % 8 != 0) {
        size += (8 - ((size % 8)));
    }

    switch (algoState) {
        case BEST_FIT:
            return bestfit(size);
        case WORST_FIT:
            return worstfit(size);
        case FIRST_FIT:
            return fnfit(freehead, size);
        case NEXT_FIT:
            return fnfit(freenext, size);
        case BUDDY:
            return buddy(size);
    }
    return 0;
}

int bfree(void *ptr) {
    header_t *hptr = (header_t *) ((void *) ptr - sizeof(header_t));
    if (hptr->magic == MAGIC) {
        node_t *curr = (node_t *) freehead;
        node_t *prev = NULL;
        do {
            //while end of freed node = start of some free node
            size_t tempsize = curr->size;
            node_t *tempnext = curr->next;
            if (((void *) curr + tempsize) == (void *) hptr) {
                curr->size += hptr->size + sizeof(header_t);
                while (((void *) curr + curr->size) == (void *) curr->next) {
                    curr->size += curr->next->size;
                    curr->next = curr->next->next;
                }
                return 0;
            } else if ((ptr + hptr->size) == (void *) curr) {
                //move curr
                curr = (node_t *) hptr;
                //grow curr
                curr->size = tempsize + hptr->size + sizeof(header_t);
                curr->next = tempnext;
                if (prev == NULL) {
                    freehead = (void *) curr;
                } else {
                    prev->next = curr;
                }
                curr = (node_t *) freehead;
                while (((void *) curr + curr->size) == (void *) curr->next) {
                    curr->size += curr->next->size;
                    curr->next = curr->next->next;
                }
                return 0;
            }
            prev = curr;
            curr = curr->next;
        } while (curr != NULL);
        size_t tempsize = hptr->size;
        node_t *tail = (node_t *) hptr;
        tail->size = tempsize + sizeof(header_t);
        tail->next = NULL;
        curr = (node_t *) freehead;
        prev = NULL;
        if (curr->size == 0) {
            freehead = (void *) tail;
            return 0;
        }
        while ((void *) curr < (void *) tail) {
            prev = curr;
            curr = curr->next;
        }
        if (prev == NULL) {
            freehead = (void *) tail;
        } else {
            prev->next = tail;
            tail->next = curr;
        }
        tail->next = curr;
        curr = (node_t *) freehead;
        while (((void *) curr + curr->size) == (void *) curr->next) {
            curr->size += curr->next->size;
            curr->next = curr->next->next;
        }
        curr = (node_t *) freehead;
        return 0;
    }
    return -1;
}

int ufree(void *ptr) {
    if (ptr == NULL) { return 0; }
    if (algoState == BUDDY) { return bfree(ptr); }
    header_t *hptr = (header_t *) ((void *) ptr - sizeof(header_t));
    if (hptr->magic == MAGIC) {
        node_t *curr = (node_t *) freehead;
        node_t *prev = NULL;
        do {
            //if end of prev free chunk lines up with start of header, join
            if (((void *) curr + curr->size) == (void *) hptr) {
                //update size of free nod
                curr->size += hptr->size + sizeof(header_t);
                coalesce();
                return 0;
            }
                //if start of free chunk lines up with start of offset from header, join
            else if ((void *) curr == (ptr + hptr->size)) {
                //copy free node
                size_t tempsize = curr->size;
                node_t *tempnext = curr->next;
                //move free node
                if (prev == NULL) {
                    curr = (node_t *) ((void *) curr - (hptr->size + sizeof(header_t)));
                    if (freenext == freehead) {
                        freenext = (void *) curr;
                    }
                    freehead = (void *) curr;
                } else {
                    curr = (node_t *) ((void *) curr - (hptr->size + sizeof(header_t)));
                    prev->next = curr;
                }
                //initialize new node
                curr->size = tempsize + hptr->size + sizeof(header_t);
                curr->next = tempnext;
                coalesce();
                return 0;
            }
            prev = curr;
            curr = curr->next;
        } while (curr != NULL);
        size_t tempsize = hptr->size;
        node_t *tail = (node_t *) hptr;
        tail->size = tempsize + sizeof(header_t);
        tail->next = NULL;
        prev->next = tail;
        coalesce();
        return 0;
    }
    printf("magic!=\n");
    return -1;
}

void *bestfit(size_t size) {
    node_t *curr = (void *) freehead;
    node_t *best = (void *) freehead;
    node_t *prev = NULL;
    size_t min = curr->size + size;
    do {
        if (curr->size > (size + sizeof(header_t))) {
            size_t tempmin = curr->size - (size + sizeof(header_t));
            if (tempmin < min) {
                min = tempmin;
                best = curr;
            }
        }
        curr = curr->next;
    } while (curr != NULL);
    //no chunk big enough
    if ((size + sizeof(header_t)) > best->size) { return NULL; }
    if ((void *) best != freehead) {
        prev = (node_t *) freehead;
        while ((prev = prev->next) != best);
    }

    if ((size + sizeof(header_t)) == best->size) {
        //if there is a previous node, link prev->next from free to free->next
        //removing free from
        header_t *header = (header_t *) best;
        if (prev != NULL) {
            prev->next = best->next;
        } else {
            //[prev=NULL=next] => only one free node size 0. (keep at least 1 node)
            if (best->next == NULL) {
                best = (node_t *) ((void *) best + size + sizeof(header_t));
                best->size = 0;
                best->next = NULL;
                freehead = (void *) best;
                freenext = (void *) best;
            }
        }
        header->size = size;
        header->magic = MAGIC;
        return (void *) header + sizeof(header_t);
    }
    if ((size + sizeof(header_t)) < best->size) {
        size_t tempsize = best->size;
        node_t *tempnext = best->next;
        //move and shrink free node
        header_t *header = (header_t *) best;
        best = (node_t *) ((void *) best + size + sizeof(header_t));
        best->size = tempsize - (size + sizeof(header_t));
        best->next = tempnext;
        //if there is a previous node, update its next
        if (prev != NULL) {
            //pointer for free has changed so update prev->next
            prev->next = best;
        } else {
            //update freehead if modifying first node
            freehead = (void *) best;
        }
        //header starts at start of curr
        header->size = size;
        header->magic = MAGIC;
        return (void *) header + sizeof(header_t);
    }
    return NULL;
}

void *worstfit(size_t size) {
    node_t *curr = (void *) freehead;
    node_t *max = (void *) freehead;
    node_t *prev = NULL;
    do {
        if (curr->size > max->size) {
            max = curr;
        }
        curr = curr->next;
    } while (curr != NULL);
    //no chunk big enough
    if ((size + sizeof(header_t)) > max->size) { return NULL; }
    if ((void *) max != freehead) {
        prev = (node_t *) freehead;
        while ((prev = prev->next) != max);
    }

    if ((size + sizeof(header_t)) == max->size) {
        //if there is a previous node, link prev->next from free to free->next
        //removing free from
        header_t *header = (header_t *) max;
        if (prev != NULL) {
            prev->next = max->next;
        } else {
            //[prev=NULL=next] => only one free node size 0. (keep at least 1 node)
            if (max->next == NULL) {
                max = (node_t *) ((void *) max + size + sizeof(header_t));
                max->size = 0;
                max->next = NULL;
                freehead = (void *) max;
                freenext = (void *) max;
            }
        }
        header->size = size;
        header->magic = MAGIC;
        return (void *) header + sizeof(header_t);
    }
    if ((size + sizeof(header_t)) < max->size) {
        size_t tempsize = max->size;
        node_t *tempnext = max->next;
        //move and shrink free node
        header_t *header = (header_t *) max;
        max = (node_t *) ((void *) max + size + sizeof(header_t));
        max->size = tempsize - (size + sizeof(header_t));
        max->next = tempnext;
        //if there is a previous node, update its next
        if (prev != NULL) {
            //pointer for free has changed so update prev->next
            prev->next = max;
        } else {
            //update freehead if modifying first node
            freehead = (void *) max;
        }
        //header starts at start of curr
        header->size = size;
        header->magic = MAGIC;
        return (void *) header + sizeof(header_t);
    }
    return NULL;
}

void *fnfit(void *_curr, size_t size) {
    node_t *curr = (node_t *) _curr;
    node_t *prev = NULL;
    if ((void *) curr != freehead) {
        prev = (node_t *) freeprev;
    }
    do {
        if (curr->next == NULL) {
            freenext = freehead;
        } else {
            freeprev = (void *) curr;
            freenext = (void *) curr->next;
        }
        //remove node
        if (curr->size == (size + sizeof(header_t))) {
            //if there is a previous node, link prev->next from free to free->next
            //removing free from
            header_t *header = (header_t *) curr;
            if (prev != NULL) {
                prev->next = curr->next;
            } else {
                //[prev=NULL=next] => only one free node size 0. (keep at least 1 node)
                if (curr->next == NULL) {
                    curr = (node_t *) ((void *) curr + size + sizeof(header_t));
                    curr->size = 0;
                    curr->next = NULL;
                    freehead = (void *) curr;
                    freenext = (void *) curr;
                }
            }
            header->size = size;
            header->magic = MAGIC;
            return (void *) header + sizeof(header_t);
        }
            //shrink node
        else if (curr->size > (size + sizeof(header_t))) {
            size_t tempsize = curr->size;
            node_t *tempnext = curr->next;
            //move and shrink free node
            header_t *header = (header_t *) curr;
            curr = (node_t *) ((void *) curr + size + sizeof(header_t));
            curr->size = tempsize - (size + sizeof(header_t));
            curr->next = tempnext;
            //if there is a previous node, update its next
            if (prev != NULL) {
                //pointer for free has changed so update prev->next
                prev->next = curr;
            } else {
                //update freehead if modifying first node
                freehead = (void *) curr;
                freenext = (void *) curr;
            }
            //header starts at start of curr
            header->size = size;
            header->magic = MAGIC;
            return (void *) header + sizeof(header_t);
        }
        prev = curr;
        curr = curr->next;
    } while (curr != NULL);
    printf("null return\n");
    //return null if no size is bigg enough or free list is null
    return NULL;
}

void *buddy(size_t size) {
    node_t *curr = (node_t *) freehead;
    node_t *prev = NULL;
    do {
        //find first freenode that is big enough
        //  avalible    >    required
        if (curr->next == NULL) {
            size += sizeof(node_t);
        }
        if (curr->size >= (size + sizeof(header_t))) {
            int shrunk = 0;
            if (curr->next == NULL) {
                size -= sizeof(node_t);
            }
            //copy curr data
            size_t ognodesize = curr->size;
            node_t *tempnext = curr->next;
            //loop until size is just big enough
            //      avalible/2    >    required
            while ((curr->size >> 1) >= (size + sizeof(header_t))) {
                //shrink
                curr->size = curr->size >> 1;
                shrunk = 1;
            }
            size_t hsize = curr->size - sizeof(header_t);
            header_t *hptr = (header_t *) curr;
            hptr->magic = MAGIC;
            hptr->size = hsize;
            //if node was shrunk
            if (shrunk) {
                //buddy of hptr
                curr = (node_t *) ((void *) hptr + hsize + sizeof(header_t));
                curr->size = hsize + sizeof(header_t);
            } else {
                //if node was not shrunk
                //if there is only one node set to size 0;
                if (tempnext == NULL && prev == NULL) {
                    curr = (node_t *) ((void *) hptr + hsize + sizeof(header_t) - sizeof(node_t));
                    curr->size = 0;
                    curr->next = NULL;
                    freehead = (void *) curr;
                }
                    //if next is null but prev is not null
                else {
                    curr = tempnext;
                }
            }
            //if first node in list
            if (prev == NULL) {
                //change freehead
                freehead = (void *) curr;
            } else {//if not
                //link previous node to new location of curr
                prev->next = curr;
            }
            //while less than half of the og size
            if (curr == NULL) {
                return (void *) hptr + sizeof(header_t);
            }
            while ((ognodesize >> 1) > curr->size) {
                if (curr->size == 0) { break; }
                //create a node at the end of current node
                curr->next = (node_t *) ((void *) curr + curr->size);
                //new node has double the size
                curr->next->size = curr->size << 1;
                //iterate
                curr = curr->next;
            }
            //link new sublist to the original next node
            if (shrunk) {
                curr->next = tempnext;
            }
            return (void *) hptr + sizeof(header_t);
        }
        prev = curr;
        curr = curr->next;
    } while (curr != NULL);
    return NULL;
}

void umemdump() {
    int i = 1;
    node_t *curr = (node_t *) freehead;
    printf("*---------MEMORY DUMP----------*\nFree:\n");
    do {
        if (algoState == BUDDY && curr->next == NULL && curr->size != 0) { curr->size -= sizeof(node_t); }
        printf("block %i addr:\t(%p)\n", i, (void *) curr);
        printf("block %i size:\t(%li)\n", i, curr->size);
        if (algoState == BUDDY && curr->next == NULL && curr->size != 0) { curr->size += sizeof(node_t); }
        i++;
        curr = curr->next;
    } while (curr != NULL);
    printf("*------------------------------*\n");
}

void coalesce() {
    node_t *curr = (node_t *) freehead;
    node_t *curr2 = (node_t *) freehead;
    node_t *prev2 = NULL;
    do {
        do {
            if (((void *) curr + curr->size) == (void *) curr2) {
                //join nodes
                curr->size += curr2->size;
                //remove curr2 from list
                if ((void *) curr2 == freehead) {
                    if (freenext == freehead) {
                        freenext = curr2->next;
                    }
                    freehead = curr2->next;
                } else {
                    prev2->next = curr2->next;
                }
            }
            prev2 = curr2;
            curr2 = curr2->next;
        } while (curr2 != NULL);
        curr2 = (node_t *) freehead;
        prev2 = NULL;
        curr = curr->next;
    } while (curr != NULL);
}