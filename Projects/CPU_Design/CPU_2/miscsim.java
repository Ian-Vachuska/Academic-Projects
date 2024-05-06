import java.io.*;
import java.util.*;

public class miscsim {
  // fields
  ArrayList<Integer> im;
  // registers
  int[] r = { 16, 0, 0, 0 };
  int[] m = new int[256];
  int pc;
  boolean zFlag;
  boolean cFlag;

  public static void main(String[] args) {
    String usage = "USAGE:  java Miscsim.java  " 
        + "<object file> [<cycles>] [-d] [-m]\n     "
        + "   -d : print disassembly listing with each cycle\n" 
        + "   -m : print memory matrix after last cycle with\n"
        + "        if cycles are unspecified "
        + "the CPU will run for 20 cycles ";
    if (args.length == 0) {
      System.out.println(usage);
    } else if (args.length == 1) {
      File file = new File(args[0]);
      if (file.exists()) {
        new miscsim(args[0], 20, "", "");
      } else {
        System.out.println(usage);
      }
    } else if (args.length == 2) {
      File file = new File(args[0]);
      if (file.exists()) {
        int maxCycles = 20;
        try {
          maxCycles = Integer.parseInt(args[1]);
          new miscsim(args[0], maxCycles, "", "");
        } catch (NumberFormatException e) {
          if (args[1].equals("-d") & maxCycles > 0) {
            new miscsim(args[0], maxCycles, args[1], "");
          } else if (args[1].equals("-m") & maxCycles > 0) {
            new miscsim(args[0], maxCycles, "", args[1]);
          } else {
            System.out.println(usage);
          }
        }
      } else {
        System.out.println(usage);
      }
    } else if (args.length == 3) {
      File file = new File(args[0]);
      if (file.exists()) {
        int maxCycles = 20;
        try {
          maxCycles = Integer.parseInt(args[1]);
          if (args[2].equals("-d") & maxCycles > 0) {
            new miscsim(args[0], maxCycles, args[2], "");
          } else if (args[2].equals("-m") & maxCycles > 0) {
            new miscsim(args[0], maxCycles, "", args[2]);
          } else {
            System.out.println(usage);
          }
        } catch (NumberFormatException e) {
          try {
            maxCycles = Integer.parseInt(args[2]);
            if (args[1].equals("-d") & maxCycles > 0) {
              new miscsim(args[0], maxCycles, args[1], "");
            } else if (args[1].equals("-m") & maxCycles > 0) {
              new miscsim(args[0], maxCycles, "", args[1]);
            } else {
              System.out.println(usage);
            }
          } catch (NumberFormatException e1) {
            if (args[1].equals("-d") 
                & args[2].equals("-m") 
                & maxCycles > 0) {
              new miscsim(args[0], maxCycles, args[1], args[2]);
            } else if (args[1].equals("-m") 
                & args[2].equals("-d") 
                & maxCycles > 0) {
              new miscsim(args[0], maxCycles, args[2], args[1]);
            } else {
              System.out.println(usage);
            }
          }
        }
      } else {
        System.out.println(usage);
      }
    } else if (args.length == 4) {
      File file = new File(args[0]);
      if (file.exists()) {
        int maxCycles = 20;
        try {
          maxCycles = Integer.parseInt(args[1]);
          if (args[2].equals("-d") 
              & args[3].equals("-m") 
              & maxCycles > 0) {
            new miscsim(args[0], maxCycles, args[2], args[3]);
          } else if (args[2].equals("-m") 
              & args[3].equals("-d") 
              & maxCycles > 0) {
            new miscsim(args[0], maxCycles, args[3], args[2]);
          } else {
            System.out.println(usage);
          }
        } catch (NumberFormatException e) {
          try {
            maxCycles = Integer.parseInt(args[2]);
            if (args[1].equals("-d") 
                & args[3].equals("-m") 
                & maxCycles > 0) {
              new miscsim(args[0], maxCycles, args[1], args[3]);
            } else if (args[1].equals("-m") 
                & args[3].equals("-d") & maxCycles > 0) {
              new miscsim(args[0], maxCycles, args[3], args[1]);
            } else {
              System.out.println(usage);
            }
          } catch (NumberFormatException e1) {
            try {
              maxCycles = Integer.parseInt(args[3]);
              if (args[1].equals("-d") 
                  & args[2].equals("-m") 
                  & maxCycles > 0) {
                new miscsim(args[0], maxCycles, args[1], args[2]);
              } else if (args[1].equals("-m") 
                  & args[2].equals("-d") 
                  & maxCycles > 0) {
                new miscsim(args[0], maxCycles, args[2], args[1]);
              } else {
                System.out.println(usage);
              }
            } catch (NumberFormatException e2) {
              System.out.println(usage);
            }
          }
        }
      } else {
        System.out.println(usage);
      }
    }
  }

  public miscsim(String inFile, int maxCycle, String dflag, String mflag) {
    try {
      BufferedReader buf;
      buf = new BufferedReader(new FileReader(inFile));
      String line;
      int instruct;
      int cycle = 0;
      im = new ArrayList<Integer>();
      line = buf.readLine();
      if (!line.equalsIgnoreCase("v2.0 raw")) {
        System.out.println("Not proper input file format");
        buf.close();
        System.exit(-1);
      } else {
        pc = 0;
        while ((line = buf.readLine()) != null) {
          im.add(Integer.parseInt(line, 16));
        }
        buf.close();
        while (cycle < maxCycle) {
          instruct = im.get(pc);
          int op = instruct << 20;
          op >>>= 30;
          int opALU = instruct << 22;
          opALU >>>= 28;
          int rn = instruct << 26;
          rn >>>= 30;
          int rm = instruct << 28;
          rm >>>= 30;
          int rd = instruct << 30;
          rd >>>= 30;
          cycle++;
          execute(op, opALU, rn, rm, rd);
          System.out.printf("Cycle:%02d State:%03X PC:%02d Z:%d C:%d " 
          + "R0: %02X R1: %02X R2: %02X  R3: %02X\n"
          + "Memory: F0[%02X] F1[%02X]\n",
              cycle, instruct, pc, Boolean.compare(zFlag, false),
              Boolean.compare(cFlag, false), (byte) r[0],
              (byte) r[1], (byte) r[2], (byte) r[3],
              (byte) m[240], (byte) m[241]);
          if (dflag.equals("-d")) {
            if (op == 0) {
              switch (opALU) {
              case 0:
                System.out.printf("Disassembly: add" 
              + " r%d r%d r%d\n\n", rd, rn, rm);
                break;
              case 1:
                System.out.printf("Disassembly: sub" 
              + " r%d r%d r%d\n\n", rd, rn, rm);
                break;
              case 2:
                System.out.printf("Disassembly: and" 
              + " r%d r%d r%d\n\n", rd, rn, rm);
                break;
              case 3:
                System.out.printf("Disassembly: or" 
              + " r%d r%d r%d\n\n", rd, rn, rm);
                break;
              case 4:
                System.out.printf("Disassembly: not" 
              + " r%d r%d\n\n", rd, rn);
                break;
              case 5:
                System.out.printf("Disassembly: shl" 
              + " r%d\n\n", rd);
                break;
              case 6:
                System.out.printf("Disassembly: asr" 
              + " r%d\n\n", rd);
                break;
              case 8:
                System.out.printf("Disassembly: mov" 
              + " r%d r%d\n\n", rd, rn);
                break;
              case 12:
                System.out.printf("Disassembly: st" 
              + " r%d [r%d]\n\n", rd, rn);
                break;
              case 13:
                System.out.printf("Disassembly: ld" 
              + " r%d [r%d]\n\n", rd, rn);
                break;
              }
            } else if (op == 1) {
              int braz = instruct << 22;
              braz >>>= 22;
              // 10-bit 2s complement -> 32-bit 2s complement
              if ((braz & 0b1000000000) == 0b1000000000) {
                braz = braz ^ 0b1111111111;
                braz += 1;
                braz = -braz;
              }
              System.out.printf("Disassembly: bnz" 
              + " %d\n\n", braz);
            } else if (op == 2) {
              int brac = instruct << 22;
              brac >>>= 22;
              // 10-bit 2s complement -> 32-bit 2s complement
              if ((brac & 0b1000000000) == 0b1000000000) {
                brac = brac ^ 0b1111111111;
                brac += 1;
                brac = -brac;
              }
              System.out.printf("Disassembly: bcs" 
              + " %d\n\n", brac);
            } else if (op == 3) {
              int immv = instruct << 22;
              immv >>>= 24;
              // 8-bit 2s complement -> 32-bit 2s complement
              if ((immv & 0b10000000) == 0b10000000) {
                immv = immv ^ 0b11111111;
                immv += 1;
                immv = -immv;
              }
              System.out.printf("Disassembly: mvi" 
              + " r%d %d\n\n", rd, immv);
            } else {
              System.out.print("bad op dis");
            }
          }
        }
      }
      if (mflag.equals("-m")) {
        outputMemory();
      }
    } catch (IOException e) {
      System.out.println("In catch");
      System.exit(-1);
    }
  }

  public void execute(int op, int opALU, int rn, int rm, int rd) {
    int temp = 0;
    if (op == 0) {
      switch (opALU) {
      case 0:// add
        r[rd] = r[rn] + r[rm];
        temp = Math.abs(r[rn]) + Math.abs(r[rm]);
        if ((0b100000000 & temp) != 0) {
          cFlag = true;
        } else {
          cFlag = false;
        }
        r[rd] = r[rd] % 256;
        zFlag = (r[rd] == 0);
        pc++;
        break;
      case 1:// sub
        r[rd] = r[rn] - r[rm];
        temp = Math.abs(r[rn]) + Math.abs(r[rm]);
        if ((0b100000000 & temp) != 0) {
          cFlag = true;
        } else {
          cFlag = false;
        }
        r[rd] = r[rd] % 256;
        zFlag = (r[rd] == 0);
        pc++;
        break;
      case 2:// and
        r[rd] = r[rn] & r[rm];
        r[rd] = r[rd] % 256;
        zFlag = (r[rd] == 0);
        pc++;
        break;
      case 3:// or
        r[rd] = r[rn] | r[rm];
        r[rd] = r[rd] % 256;
        zFlag = (r[rd] == 0);
        pc++;
        break;
      case 4:// not
        r[rd] = r[rn] ^ 0b11111111;
        r[rd] = r[rd] % 256;
        zFlag = (r[rd] == 0);
        pc++;
        break;
      case 5:// shl
        r[rd] <<= 1;
        r[rd] = r[rd] % 256;
        zFlag = (r[rd] == 0);
        pc++;
        break;
      case 6:// asr
        r[rd] >>= 1;
        r[rd] = r[rd] % 256;
        zFlag = (r[rd] == 0);
        pc++;
        break;
      case 8:// mov
        r[rd] = r[rn];
        pc++;
        break;
      case 12:// st
        if (r[rn] < 0) {
          m[256 + r[rn]] = r[rd];
        } else {
          m[r[rn]] = r[rd];
        }
        pc++;
        break;
      case 13:// ld
        if (r[rn] < 0) {
          r[rd] = m[256 + r[rn]];
        } else {
          r[rd] = m[r[rn]];
        }
        pc++;
        break;
      }

    }
    int bitmask = 0b1000000000;
    switch (op) {
    case 1:// bnz
      if (!zFlag) {
        int bta = opALU << 2;
        bta = bta | rn;
        bta <<= 2;
        bta = bta | rm;
        bta <<= 2;
        bta = bta | rd;
        if ((bta & bitmask) == bitmask) {
          bta = bta ^ 0b1111111111;
          bta += 1;
          bta = -bta;
        }
        pc += bta;
      } else {
        pc++;
      }
      break;
    case 2:// bcs
      if (cFlag) {
        int bta = opALU << 2;
        bta = bta | rn;
        bta <<= 2;
        bta = bta | rm;
        bta <<= 2;
        bta = bta | rd;
        if ((bta & bitmask) == bitmask) {
          bta = bta ^ 0b1111111111;
          bta += 1;
          bta = -bta;
        }
        pc += bta;
      } else {
        pc++;
      }
      break;
    case 3:// mvi
      int imm = opALU << 2;
      imm = imm | rn;
      imm <<= 2;
      imm = imm | rm;
      if ((imm & 0b10000000) == 0b10000000) {
        imm = imm ^ 0b11111111;
        imm += 1;
        imm = -imm;
      }
      r[rd] = imm;
      pc++;
      break;
    }
  }

  // Outputs memory matrix after last cycle executed
  private void outputMemory() {
    int k = 0;
    System.out.printf("\nMemory Output(-m) after last cycle execution:\n" 
    + "Format: address[value]\n\n");
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        System.out.printf("%02x[%02X]  ", k, (byte) m[k]);
        k++;
      }
      System.out.printf("\n\n");
    }
  }
}
