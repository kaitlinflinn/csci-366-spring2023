//
// Created by carson on 11/15/21.
//

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "assembler.h"

char *ASM_ERROR_UNKNOWN_INSTRUCTION = "Unknown Assembly Instruction";
char *ASM_ERROR_ARG_REQUIRED = "Argument Required";
char *ASM_ERROR_BAD_LABEL = "Bad Label";
char *ASM_ERROR_OUT_OF_RANGE = "Number is out of range";

//=========================================================
//  All the instructions available on the LMSM architecture
//=========================================================
const char *INSTRUCTIONS[28] =
        {"ADD", "SUB", "LDA", "STA", "BRA", "BRZ", "BRP", "INP", "OUT", "HLT", "COB", "DAT",
         "LDI",
         "JAL", "CALL", "RET",
         "SPUSH", "SPUSHI", "SPOP", "SDUP", "SDROP", "SSWAP", "SADD", "SSUB", "SMAX", "SMIN", "SMUL", "SDIV"
        };
const int INSTRUCTION_COUNT = 28;

//===================================================================
//  All the instructions that require an arg on the LMSM architecture
//===================================================================
const char *ARG_INSTRUCTIONS[11] =
        {"ADD", "SUB", "LDA", "STA", "BRA", "BRZ", "BRP", "DAT",
         "LDI",
         "CALL",
         "SPUSHI"
        };
const int ARG_INSTRUCTION_COUNT = 11;

//======================================================
// Constructors/Destructors
//======================================================

asm_instruction * asm_make_instruction(char* type, char *label, char *label_reference, int value, asm_instruction * predecessor) {
    asm_instruction *new_instruction = calloc(1, sizeof(asm_instruction));
    new_instruction->instruction = type;
    new_instruction->label = label;
    new_instruction->label_reference = label_reference;
    new_instruction->value = value;
    new_instruction->next = NULL;
    if (predecessor != NULL) {
        predecessor->next = new_instruction;
        new_instruction->offset = predecessor->offset + predecessor->slots;
    } else {
        new_instruction->offset = 0;
    }
    // TODO: set the number of slots for the instruction into the slots field
    if(strcmp(type, "SPUSHI") == 0){
        new_instruction->slots = 2;
    }
    else if(strcmp(type, "CALL")==0){
        new_instruction->slots = 3;
    }
    else{
        new_instruction->slots=1;
    }
    return new_instruction;
}

void asm_delete_instruction(asm_instruction *instruction) {
    if (instruction == NULL) {
        return;
    }
    asm_delete_instruction(instruction->next);
    free(instruction);
}

asm_compilation_result * asm_make_compilation_result() {
    asm_compilation_result *result = calloc(1, sizeof(asm_compilation_result));
    return result;
}

void asm_delete_compilation_result(asm_compilation_result *result) {
    asm_delete_instruction(result->root);
    free(result);
}

//======================================================
// Helpers
//======================================================
int asm_is_instruction(char * token) {
    for (int i = 0; i < INSTRUCTION_COUNT; ++i) {
        if (strcmp(token, INSTRUCTIONS[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int asm_instruction_requires_arg(char * token) {
    for (int i = 0; i < ARG_INSTRUCTION_COUNT; ++i) {
        if (strcmp(token, ARG_INSTRUCTIONS[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int asm_is_num(char * token){
    if (*token == '-') { // allow a leading negative
        token++;
        if (*token == '\0') {
            return 0;
        }
    }
    while (*token != '\0') {
        if (*token < '0' || '9' < *token) {
            return 0;
        }
        token++;
    }
    return 1;
}

int asm_find_label(asm_instruction *root, char *label) {
    // TODO - scan the linked list for the given label, return -1 if not found
    if(root->label == label){
        return root->offset;
    } else if(root->next != NULL){
        return asm_find_label(root->next, label);
    } else if(root->next == NULL){
        return -1;
    }
    return -1;
}


//======================================================
// Assembly Parsing/Scanning
//======================================================

void asm_parse_src(asm_compilation_result * result, char * original_src){

    // copy over so strtok can mutate
    char * src = calloc(strlen(original_src) + 1, sizeof(char));
    strcat(src, original_src);
    asm_instruction * last_instruction = NULL;
    asm_instruction * current_instruction = NULL;

    char *token = strtok(src, " \n");
    //<label> <inst>
    //<label> <inst> <args>
    //<inst>
    //<inst> <args>
    while(token != NULL) {
        char *label = NULL;
        char *instruction = NULL;
        char* label_reference = NULL;
        int arg_value = 0;

        int is_inst = asm_is_instruction(token);
        if (!is_inst) { //isn't an instruction, must be a label
            label = token;
            token = strtok(NULL, "\n"); //grab next part
            //CASE 1: <label> <inst>
            if(asm_is_instruction(token)){
                instruction = token;
                current_instruction = asm_make_instruction(instruction, label, label_reference, arg_value, last_instruction);
                //advance to next token
                token = strtok(NULL, "\n");
            } else if(!asm_is_instruction(token)){ //two labels
                result->error = ASM_ERROR_UNKNOWN_INSTRUCTION;
                break;
            }
            //CASE 2: <label> <inst> <args>
            else{
                instruction = token;
                if (strtok(NULL, "\n") == NULL || !asm_is_instruction(strtok(NULL, "\n")) || !asm_is_num(strtok(NULL, "\n"))) {
                    result->error = ASM_ERROR_ARG_REQUIRED;
                    return;
                } else{
                    token = strtok(NULL, "\n"); //token is now argument
                    if(!asm_is_num(token)) { //see if it is an int
                        arg_value = atoi(token);
                    }else{
                        label_reference = token;
                    }
                }
                current_instruction = asm_make_instruction(instruction, label, label_reference, arg_value, last_instruction);
                //advance to next token
                token = strtok(NULL, "\n");
            }
        }
        else { // is_inst passes, is an instruction
            instruction = token; //assign to instruction variable
            token = strtok(NULL, "\n"); //grab next part
        }
        //test if instruction needs argument
        //CASE 4: <inst> <args>
        int requires_args = asm_instruction_requires_arg(instruction);
        if(requires_args) { //if instruction needs args, tokenize and get args
            if (token == NULL) {   // asm_is_instruction(token) || !asm_is_num(token)) {
                result->error = ASM_ERROR_ARG_REQUIRED;
                return;
            } else { //token is label
                if (asm_is_num(token)) { //see if it is an int
                    arg_value = atoi(token);
                    if(arg_value > 999 || arg_value < -999){
                        result->error = ASM_ERROR_OUT_OF_RANGE;
                        break;
                    }
                } else {
                    label_reference = token;
                }
            }
            current_instruction = asm_make_instruction(instruction, label, label_reference, arg_value,
                                                       last_instruction);
            //advance to next token
            token = strtok(NULL, "\n");
        }
        //CASE 3: <inst>
        else if(token == NULL && instruction != NULL){ //only one instruction in line
            current_instruction = asm_make_instruction(instruction, label, label_reference, arg_value, last_instruction);
        } else if(token == NULL || !asm_is_instruction(token)) { //looking at second part of line
            result->error = ASM_ERROR_UNKNOWN_INSTRUCTION;
            return;
        } else{
            current_instruction = asm_make_instruction(instruction, label, label_reference, arg_value,
                                                       last_instruction);
        }

        //check to see if we need to create root
        if(result->root == NULL){
            result->root = current_instruction;
        }
        last_instruction = current_instruction;


    }

    //TODO - generate a linked list of instructions and store the first into
    //       the result->root
    //
    //       generate the following errors as appropriate:
    //
    //       ASM_ERROR_UNKNOWN_INSTRUCTION - when an unknown instruction is encountered
    //       ASM_ERROR_ARG_REQUIRED        - when an instruction does not have a proper argument passed to it
    //       ASM_ERROR_OUT_OF_RANGE        - when a number argument is out of range (-999 to 999)
    //
    //       store the error in result->error
}

//======================================================
// Machine Code Generation
//======================================================

void asm_gen_code_for_instruction(asm_compilation_result  * result, asm_instruction *instruction) {
    //TODO - generate the machine code for the given instruction
    //
    // note that some instructions will take up multiple slots
    //
    // note that if the instruction has a label reference rather than a raw number reference
    // you will need to look it up with `asm_find_label` and, if the label does not exist,
    // report the error as ASM_ERROR_BAD_LABEL


    int value_for_instruction = instruction->value;
    //if value_for_instruction is a label
//    if(value_for_instruction == 0){
//        //point to correct spot in memory
//       // int offset = asm_find_label(instruction, instruction->label_reference);
//       // instruction->label = result->code[offset];
//    }

    if (strcmp("ADD", instruction->instruction) == 0) {
        result->code[instruction->offset] = 100 + value_for_instruction;
    } else if (strcmp("SUB", instruction->instruction) == 0) {
        result->code[instruction->offset] = 200 + value_for_instruction;
    } else if (strcmp("STA", instruction->instruction) == 0) {
        result->code[instruction->offset] = 300 + value_for_instruction;
    } else if (strcmp("LDI", instruction->instruction) == 0) {
        result->code[instruction->offset] = 400 + value_for_instruction;
    } else if (strcmp("LDA", instruction->instruction) == 0) {
        if(instruction->label_reference == NULL){
            result->code[instruction->offset] = 500 + value_for_instruction;
        } else { //LDA has a label reference, need to find the label
            int num = asm_find_label(result->root, instruction->label_reference);
            if(num == -1){
                result->error = ASM_ERROR_BAD_LABEL;
            }
            result->code[instruction->offset] = 500 + num;
        }


    } else if (strcmp("BRA", instruction->instruction) == 0) {
        result->code[instruction->offset] = 600 + value_for_instruction;
    } else if (strcmp("BRZ", instruction->instruction) == 0) {
        result->code[instruction->offset] = 700 + value_for_instruction;
    } else if (strcmp("BRP", instruction->instruction) == 0) {
        result->code[instruction->offset] = 800 + value_for_instruction;
    } else if (strcmp("INP", instruction->instruction) == 0) {
        result->code[instruction->offset] = 901;
    } else if (strcmp("OUT", instruction->instruction) == 0) {
        result->code[instruction->offset] = 902;
    } else if (strcmp("HALT", instruction->instruction) == 0 || strcmp("COB", instruction->instruction) == 0) {
        result->code[instruction->offset] = 000;
    } else if(strcmp("DAT", instruction->instruction) == 0){
        result->code[instruction->offset] = instruction->value;
    } else if(strcmp("RET", instruction->instruction) == 0){
        result->code[instruction->offset] = 911;
    }
    //STACK INSTRUCTIONS
    else if (strcmp("SPUSH", instruction->instruction) == 0) {
        result->code[instruction->offset] = 920;
    } else if (strcmp("SPOP", instruction->instruction) == 0) {
        result->code[instruction->offset] = 921;
    } else if (strcmp("SDUP", instruction->instruction) == 0) {
        result->code[instruction->offset] = 922;
    } else if (strcmp("SDROP", instruction->instruction) == 0) {
        result->code[instruction->offset] = 923;
    } else if (strcmp("SSWAP", instruction->instruction) == 0) {
        result->code[instruction->offset] = 924;
    } else if (strcmp("SADD", instruction->instruction) == 0) {
        result->code[instruction->offset] = 930;
    } else if (strcmp("SSUB", instruction->instruction) == 0) {
        result->code[instruction->offset] = 931;
    } else if (strcmp("SMUL", instruction->instruction) == 0) {
        result->code[instruction->offset] = 932;
    } else if (strcmp("SDIV", instruction->instruction) == 0) {
        result->code[instruction->offset] = 933;
    } else if (strcmp("SMAX", instruction->instruction) == 0) {
        result->code[instruction->offset] = 934;
    } else if (strcmp("SMIN", instruction->instruction) == 0) {
        result->code[instruction->offset] = 935;
    } else if (strcmp("SPUSHI", instruction->instruction) == 0) {
        result->code[instruction->offset] = 400 + value_for_instruction;
        result->code[instruction->offset + 1] = 920;
    } else if (strcmp("CALL", instruction->instruction) == 0) {
        result->code[instruction->offset] = 400 + value_for_instruction;
        result->code[instruction->offset + 1] = 920;
        result->code[instruction->offset + 2] = 910;
    }


    else {
        result->code[instruction->offset] = 0;
    }

}

void asm_gen_code(asm_compilation_result * result) {
    asm_instruction * current = result->root;
    while (current != NULL) {
        asm_gen_code_for_instruction(result, current);
        current = current->next;
    }
}

//======================================================
// Main API
//======================================================

asm_compilation_result * asm_assemble(char *src) {
    asm_compilation_result * result = asm_make_compilation_result();
    asm_parse_src(result, src);
    asm_gen_code(result);
    return result;
}

