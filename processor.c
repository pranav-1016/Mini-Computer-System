int Register[256];
int PC, opcode, dest, src1, src2;
extern char Instruction[256];
extern int Data[256];
int end_of_simulation = 0;


void reset() {
    // Set all the register values to 0
    for (int i=0; i < 256; i++) {
        Register[i] = 0;
    }
    PC = 0;
    opcode = 0;
    dest = 0;
    src1 = 0;
    src2 = 0;
}

void fetch() {
    opcode = Instruction[PC];
    dest = Instruction[PC + 1];
    src1 = Instruction[PC + 2];
    src2 = Instruction[PC + 3];

    PC = PC + 4;
}

void decode() {}

void execute()
{
    switch (opcode)
    {
        case 0:
            end_of_simulation = 1;
            break;

        case 1:
            Register[dest] = Register[src1] + Register[src2];
            // printf("Value of the registers %d %d \n", Register[src1], Register[src2]);
            // printf("Addition : %d src1: %d src2 : %d \n", Register[dest], src1, src2);
            break;

        case 2:
            Register[dest] = Register[src1] - Register[src2];
            break;

        case 3:
            Register[dest] = Register[src1] * Register[src2];
            break;

        case 4:
            Register[dest] = Register[src1] / Register[src2];
            break;

        case 5:
            Register[dest] = Data[src1];
            printf("Read operation : %d %d\n", src1, Data[src1] );
            break;

        case 6:
            Data[src1] = Register[dest];
            break;

        case 7:
            Register[dest] = src1;
            break;

        default:
            printf("Invalid opcode: %d\n", opcode);
            end_of_simulation = 1;
            break;
    }
}