import java.io.*;
import java.util.*;

public class miscas {
  private ArrayList<Integer> outputs;
  private ArrayList<Instruction> instrucs;

  public static void main(String[] args) {
    String usage = "USAGE:  java Miscas.java " 
        + "<source file> <object file> [-l]\n"
        + "   -l : print listing to standard error";
    if (args.length == 0) {
      System.out.println(usage);
    } else if (args.length == 2) {
      File tempFile = new File(args[0]);
      if (tempFile.exists()) {
        new miscas(args[0], args[1], "");
      } else {
        System.out.println(usage);
        // input file does not exist
      }
    } else if (args.length == 3) {
      File tempFile = new File(args[0]);
      if (tempFile.exists() && args[2].equals("-l")) {
        new miscas(args[0], args[1], args[2]);
      } else {
        System.out.println(usage);
        // input file does not exist or wrong flag
      }
    } else {
      System.out.println(usage);
    }
  }

  public miscas(String inFile, String outFile, String listFlag) {
    instrucs = new ArrayList<Instruction>();
    try {
      BufferedReader buf;
      buf = new BufferedReader(new FileReader(inFile));
      String line;
      // First Pass
      while ((line = buf.readLine()) != null) {
        Instruction cur = new Instruction(line);
        if (cur.errorFree()) {
          instrucs.add(cur);
        }
      }
      buf.close();
      outputs = new ArrayList<Integer>();
      // Second Pass
      for (int i = 0; i < Math.min(instrucs.size(), 8192); i++) {
        outputs.add(i, Assemble(instrucs.get(i)));
      }
    } catch (IOException e) {
      System.out.println("Input File Error from catch");
      System.exit(-1);
    }

    // write to file
    try {
      File outputFile = new File(outFile);
      if (outputFile.createNewFile()) {
        System.out.println("File Created");
      }
      FileWriter fr = new FileWriter(outputFile);
      fr.write("v2.0 raw\n");
      for (int i = 0; i < outputs.size(); i++) {
        int h2 = outputs.get(i).intValue() << 20;
        h2 >>>= 28;
        int h1 = outputs.get(i).intValue() << 24;
        h1 >>>= 28;
        int h0 = outputs.get(i).intValue() << 28;
        h0 >>>= 28;
        fr.write(Integer.toHexString(h2).toUpperCase());
        fr.write(Integer.toHexString(h1).toUpperCase());
        fr.write(Integer.toHexString(h0).toUpperCase());
        if (i < outputs.size() - 1) {
          fr.write("\n");
        }
      }
      createOutputListing();
      fr.close();
    } catch (IOException e) {
      System.out.println("Output File Error from catch");
      System.exit(-1);
    }
  }

  public void createOutputListing() {
    System.out.println("*** MACHINE PROGRAM ***");
    for (int i = 0; i < instrucs.size(); i++) {
      System.out.printf("%02d:%03X  %s%n", i + 1,
          outputs.get(i).intValue(),
          instrucs.get(i).getInstructionWord());
    }

  }

  private Integer PassRegisters(Instruction instruct, int numOfOperands) {
    int rd = 0;
    int rn = 0;
    int rm = 0;
    if (instruct.getOperand1().equalsIgnoreCase("r0")) {
      rd = 0;
    } else if (instruct.getOperand1().equalsIgnoreCase("r1")) {
      rd = 1;
    } else if (instruct.getOperand1().equalsIgnoreCase("r2")) {
      rd = 2;
    } else if (instruct.getOperand1().equalsIgnoreCase("r3")) {
      rd = 3;
    } else {
      System.out.printf("%s is not a valid rd register",
          instruct.getOperand1());
      System.exit(-1);
    }
    if (numOfOperands >= 2) {
      if (instruct.getOperand2().equalsIgnoreCase("r0")) {
        rn = 0;
      } else if (instruct.getOperand2().equalsIgnoreCase("r1")) {
        rn = 1;
      } else if (instruct.getOperand2().equalsIgnoreCase("r2")) {
        rn = 2;
      } else if (instruct.getOperand2().equalsIgnoreCase("r3")) {
        rn = 3;
      } else {
        // invalid register error
        System.out.printf("%s is not a valid rn register",
            instruct.getOperand2());
        System.exit(-1);
      }
      if (numOfOperands == 3) {
        if (instruct.getOperand3().equalsIgnoreCase("r0")) {
          rm = 0;
        } else if (instruct.getOperand3().equalsIgnoreCase("r1")) {
          rm = 1;
        } else if (instruct.getOperand3().equalsIgnoreCase("r2")) {
          rm = 2;
        } else if (instruct.getOperand3().equalsIgnoreCase("r3")) {
          rm = 3;
        } else {
          // invalid register error
          System.out.printf("%s is not a valid rn register",
              instruct.getOperand2());
          System.exit(-1);
        }
      }
    }
    rn <<= 4;
    rm <<= 2;
    return new Integer(rn + rm + rd);
  }

  private Integer Assemble(Instruction instruct) {
    int op = 0;
    int opALU = 0;
    int bra = 0;
    int imm = 0;
    int rd = 0;
    if (instruct.getOperation().equalsIgnoreCase("add")) {
      return new Integer(PassRegisters(instruct, 3));
    } else if (instruct.getOperation().equalsIgnoreCase("sub")) {
      opALU = 1 << 6;
      return new Integer(opALU + PassRegisters(instruct, 3));
    } else if (instruct.getOperation().equalsIgnoreCase("and")) {
      opALU = 2 << 6;
      return new Integer(opALU + PassRegisters(instruct, 3));
    } else if (instruct.getOperation().equalsIgnoreCase("or")) {
      opALU = 3 << 6;
      return new Integer(opALU + PassRegisters(instruct, 3));
    } else if (instruct.getOperation().equalsIgnoreCase("not")) {
      opALU = 4 << 6;
      return new Integer(opALU + PassRegisters(instruct, 2));
    } else if (instruct.getOperation().equalsIgnoreCase("shl")) {
      opALU = 5 << 6;
      return new Integer(opALU + PassRegisters(instruct, 1));
    } else if (instruct.getOperation().equalsIgnoreCase("asr")) {
      opALU = 6 << 6;
      return new Integer(opALU + PassRegisters(instruct, 1));
    } else if (instruct.getOperation().equalsIgnoreCase("mov")) {
      opALU = 8 << 6;
      return new Integer(opALU + PassRegisters(instruct, 2));
    } else if (instruct.getOperation().equalsIgnoreCase("st")) {
      opALU = 12 << 6;
      return new Integer(opALU + PassRegisters(instruct, 2));
    } else if (instruct.getOperation().equalsIgnoreCase("ld")) {
      opALU = 13 << 6;
      return new Integer(opALU + PassRegisters(instruct, 2));
    }
    // BRANCH ZERO
    else if (instruct.getOperation().equalsIgnoreCase("bnz")) {
      op = 1 << 10;
      bra = instruct.getRelativeBranch();
      // if relative branch address is negative
      // convert 32-bit 2s complement-> 10-bit 2s complement
      if (bra < 0) {
        bra = -bra;
        bra = bra ^ 0b1111111111;
        bra += 1;
        bra = bra % 1024;
      }
      return new Integer(op + bra);
    }
    // BRANCH CARRY
    else if (instruct.getOperation().equalsIgnoreCase("bcs")) {
      op = 2 << 10;
      bra = instruct.getRelativeBranch();
      // if relative branch address is negative
      // convert 32-bit 2s complement-> 10-bit 2s complement
      if (bra < 0) {
        bra = -bra;
        bra = bra ^ 0b1111111111;
        bra += 1;
        bra = bra % 1024;
      }
      return new Integer(op + bra);
    }
    // MOVE IMMEDIATE
    else if (instruct.getOperation().equalsIgnoreCase("mvi")) {
      op = 3 << 10;
      imm = instruct.getImmediate();
      // if relative immediate value is negative
      // convert 32-bit 2s complement-> 8-bit 2s complement
      if ((imm & 0b10000000) == 0b10000000) {
        imm = -imm;
        imm = imm ^ 0b11111111;
        imm += 1;
        imm = imm % 256;
      }
      imm <<= 2;
      rd = PassRegisters(instruct, 1);
      return new Integer(op + imm + rd);
    } else {
      System.out.printf("%s is not a valid operation",
          instruct.getOperation());
      System.exit(-1);
    }
    // should not get here
    return new Integer(-1);
  }

  private class Instruction {
    // Fields
    private String iw = "";
    private String operation = "";
    private String op1 = "";
    private String op2 = "";
    private String op3 = "";
    private int imm = 0;
    private int bra = 0;
    private Boolean errorFreeFlag = false;

    public Instruction(String inputLine) {
      parseOperands(parseOperation(removeComments(inputLine)));
    }

    // Removes comments
    public String removeComments(String inputLine) {
      if (inputLine.indexOf(";") == -1) {
        iw = inputLine;
      } else {
        iw = inputLine.substring(0, inputLine.indexOf(";"));
      }
      iw = iw.trim();
      if (iw.length() < 4) {
        return iw = "";
      }
      return iw;
    }

    // Parse Operation into field and returns
    public String parseOperation(String instruction) {
      if (instruction == "") {
        return "";
      } else {
        operation = instruction.substring(0, instruction.indexOf(" "));
        return instruction.substring(instruction.indexOf(" ") + 1);
      }
    }

    // Parse list of operands based off given operation
    public void parseOperands(String operands) {
      if (operands != "") {
        // If Instruction = (add/sub/and/or) process for all 3 operands
        if (operation.equalsIgnoreCase("add") 
            || operation.equalsIgnoreCase("sub") 
            || operation.equalsIgnoreCase("and")
            || operation.equalsIgnoreCase("or")) {
          op1 = operands.substring(0, operands.indexOf(" "));
          operands = operands.substring(operands.indexOf(" ") + 1);
          op2 = operands.substring(0, operands.indexOf(" "));
          op3 = operands.substring(operands.indexOf(" ") + 1).trim();
          errorFreeFlag = true;
        }
        // If Instruction = (not/mov/mvi) process 2 operands
        else if (this.operation.equalsIgnoreCase("not") 
            || operation.equalsIgnoreCase("mov")) {
          op1 = operands.substring(0, operands.indexOf(" "));
          op2 = operands.substring(operands.indexOf(" ") + 1).trim();
          errorFreeFlag = true;
        } else if (operation.equalsIgnoreCase("mvi")) {
          op1 = operands.substring(0, operands.indexOf(" "));
          op2 = operands.substring(operands.indexOf(" ") + 1);
          if (op2.endsWith(" ") || op2.endsWith("\t")) {
            op2 = op2.trim();
          }
          try {
            imm = Integer.parseInt(op2);
            if (imm >= 128 || imm < -128) {
              System.out.printf("bad imm value size: %d\n", imm);
              System.exit(-1);
            }
          } catch (NumberFormatException e) {
            System.out.printf("bad imm input\n");
            System.exit(-1);
          }
          errorFreeFlag = true;
        } else if (operation.equalsIgnoreCase("shl") 
            || operation.equalsIgnoreCase("asr")) {
          op1 = operands;
          if (op1.endsWith(" ") || op1.endsWith("\t")) {
            op1 = op1.trim();
          }
          errorFreeFlag = true;
        }
        // If Instruction = (st/ld) process for branch target address
        else if (operation.equalsIgnoreCase("st")) {
          op1 = operands.substring(0, operands.indexOf(" "));
          op2 = operands.substring(operands.indexOf(" ") + 2,
              operands.length() - 1);
          errorFreeFlag = true;
        }
        // If Instruction = (st/ld) process for branch target address
        else if (operation.equalsIgnoreCase("ld")) {
          op1 = operands.substring(0, operands.indexOf(" "));
          op2 = operands.substring(operands.indexOf(" ") + 2,
              operands.length() - 1);
          errorFreeFlag = true;
        }
        // If Instruction = (bnz/bcs) process for branch target address
        else if (this.operation.equalsIgnoreCase("bnz") 
            || operation.equalsIgnoreCase("bcs")) {
          String str = operands;
          if (operands.endsWith(" ") 
              || operands.endsWith("\t")) {
            str = operands.trim();
          }
          try {
            bra = Integer.parseInt(str);
            if (bra >= 512 || bra < -512) {
              System.out.printf("Bad Branch size: %d\n" 
            + "-512 <= branch value < 512 allowed", bra);
              System.exit(-1);
            }
          } catch (NumberFormatException e) {
            System.out.printf("Bad Branch input\n");
            System.exit(-1);
          }
          errorFreeFlag = true;
        }
        // Invalid operation
        else {
          // instruction parsing error
          System.out.println("Not suppose to be here: Bad Operation");
        }
      }
    }

    public boolean errorFree() {
      return errorFreeFlag;
    }

    public String getOperation() {
      return operation;
    }

    public String getOperand1() {
      return op1;
    }

    public String getOperand2() {
      return op2;
    }

    public String getOperand3() {
      return op3;
    }

    public int getRelativeBranch() {
      return bra;
    }

    public int getImmediate() {
      return imm;
    }

    public String getInstructionWord() {
      return iw.trim();
    }
  }
}