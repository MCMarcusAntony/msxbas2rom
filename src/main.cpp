//******************************************************************************
//  PROJECT.......: MSXBAS2ROM
//  COPYRIGHT.....: Copyright (C) 2020- Amaury S P Carvalho
//  VERSION.......: 0.1
//  DESCRIPTION...: MSX BASIC to ROM converter
//  HOME PAGE.....: http://launchpad.net/msxbas2rom
//  CONTACT EMAIL.: amauryspires@gmail.com
//  HISTORY.......:
//  PERSON          DATE         COMMENT
//  --------        ----------   -----------------------------------------------
//  Amaury          21/05/2020   Initial version created
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <iostream>
#include <ctype.h>
#ifndef MacOS
  #include <malloc.h>
#endif
#include <math.h>

#include "main.h"

using namespace std;

int main(int argc, char *argv[]) {
    int i;
    char *s;
    Lexer lexer;
    Tokenizer tokenizer;
    Rom rom;
    Parser parser;
    Compiler compiler;
    CompilerPT3 compilerPT3;

    // parsing parameters

    for(i = 1; i < argc; i++) {
        if( argv[i][0] == '/' || argv[i][0] == '-' ) {
            switch( argv[i][1] ) {
                case 'h':
                case 'H':
                case '?':
                    parmHelp = true;
                    break;
                case 'd':
                case 'D':
                    parmDebug = true;
                    break;
                case 'q':
                case 'Q':
                    parmQuiet = true;
                    break;
                case 't':
                case 'T':
                    parmTurbo = true;
                    break;
                case 'x':
                case 'X':
                    parmXtd = true;
                    break;
                case 'c':
                case 'C':
                    parmCompile = true;
                    break;
                case 's':
                case 'S':
                    parmSymbols = true;
                    break;
                case '-': {
                        if (argv[i][2] == 'v' && argv[i][3] == 'e' && argv[i][4] == 'r' ) {
                            parmVer = true;
                        } else if (argv[i][2] == 'd' && argv[i][3] == 'o' && argv[i][4] == 'c' ) {
                            parmDoc = true;
                        } else if (argv[i][2] == 'n' && argv[i][3] == 's' && argv[i][4] == 'r' ) {
                            parmNoStripRemLines = true;
                        } else if (argv[i][2] == 'l' && argv[i][3] == 'i' && argv[i][4] == 'n' ) {
                            parmLineNumber = true;
                        }
                    }
                    break;
                default:
                    parmError = true;
                    break;
            }

        } else {

            strcpy(inputFilename, argv[i]);

        }
    }

    if(! parmQuiet) {
        // splash screen

        printf("%s %s\n", info_splash, app_version);

        // help hint
        if (parmDoc) {
            printf("%s", info_documentation);
            printf("%s\n", info_support);
            return 0;
        } else if (parmVer) {
            printf("%s", info_history);
            printf("%s\n", info_support);
            return 0;
        } else if(argc == 1 || parmHelp) {
            printf("%s", info_usage);
            printf("%s\n", info_support);
            return 0;
        } else {
        }
    } else {
        if (argc == 1 || parmHelp || parmError)
            return 1;
    }

    if(parmError) {
        printf("ERROR: Invalid parameter(s)\n");
        return 1;
    }

    if( ! inputFilename[0] ) {
        printf("ERROR: Input file name parameter is missing!\n");
        return 1;
    }

    if(! FileExists(inputFilename)) {
        printf("ERROR: Input file not found!\n");
        return 1;
    }

    // set output file name from input name

    strcpy(outputFilename, inputFilename);
    strcpy(symbolFilename, inputFilename);

    s = strrchr(outputFilename, '.');
    if(s) {
        strcpy(s, ".rom");
    } else {
        strcat(outputFilename, ".rom");
    }

    s = strrchr(symbolFilename, '.');
    if(s) {
        strcpy(s, ".symbol");
    } else {
        strcat(symbolFilename, ".symbol");
    }

    if(FileExists(outputFilename)) {
        remove(outputFilename);
    }

    if(! parmQuiet)
        printf("Converting %s to %s ...\n", inputFilename, outputFilename);

    // LEXICAL ANALYSIS
    // lexing the input file, tokenizing it

    if(! parmQuiet)
        printf("(1) Doing lexical analysis...\n");

    if( ! lexer.load(inputFilename) ) {
        printf("ERROR: Cannot load input file for lexical analysis\n");
        printf("%s\n", lexer.errorMessage.c_str());
        return 1;
    }

    if( ! lexer.evaluate() ) {
        lexer.error();
        printf("ERROR: Lexical error at line %i\n", lexer.lineNo);
        printf("%s\n", lexer.errorMessage.c_str());
        return 1;
    }

    if( parmDebug ) {
        printf("Displaying lexical analysis:\n");
        lexer.print();
    }

    parser.debug = parmDebug;

    if(parmCompile) {

        // SYNTACTIC ANALYSIS
        // parsing the lexing tokens, building the syntax tree

        if(! parmQuiet)
            printf("(2) Doing syntactic analysis...\n");

        if( ! parser.evaluate(&lexer) ) {
            if(parser.lineNo == 0)
                printf("ERROR: Cannot build the symbol tree\n");
            else {
                parser.error();
                printf("ERROR: Syntax error at line %i\n", parser.lineNo);
            }
            return 1;
        }

        if( parmDebug ) {
            printf("Displaying syntactic analysis:\n");
            parser.print();
        }

        // SEMANTIC ANALYSIS
        // create assembly output

        if(! parmQuiet)
            printf("(3) Doing semantic analysis (compiling)...\n");

        if(parser.has_akm && parser.has_pt3) {
            printf("ERROR: Cannot compile PT3 and Arkos players in the same ROM\n");
            return 1;
        }

        if(parser.has_pt3) {

            compilerPT3.megaROM = parmXtd;
            compilerPT3.debug = parmDebug;
            compilerPT3.has_line_number = parmLineNumber;

            if(!compilerPT3.build(&parser)) {
                printf("Error: %s\n", compilerPT3.error_message.c_str());
                if(compilerPT3.current_tag)
                    compilerPT3.current_tag->print();
                return 1;
            }

            if(! parmQuiet) {
                if(compilerPT3.megaROM) {
                    printf("    Compiling for MegaROM format (Konami with SCC mapper)\n");
                }
                printf("    Compiled code size = %i byte(s)\n", compilerPT3.code_size);
                printf("    Memory allocated to variables = %i byte(s)\n", compilerPT3.ram_size);
            }

        } else {

            compiler.megaROM = parmXtd;
            compiler.debug = parmDebug;
            compiler.has_line_number = parmLineNumber;

            if(!compiler.build(&parser)) {
                printf("Error: %s\n", compiler.error_message.c_str());
                if(compiler.current_tag)
                    compiler.current_tag->print();
                return 1;
            }

            if(! parmQuiet) {
                if(compiler.megaROM) {
                    printf("    Compiling for MegaROM format (Konami with SCC mapper)\n");
                }
                printf("    Compiled code size = %i byte(s)\n", compiler.code_size);
                printf("    Memory allocated to variables = %i byte(s)\n", compiler.ram_size);
            }

        }

        // ROM OUTPUT

        if(! parmQuiet)
            printf("(4) Building ROM...\n");

        rom.xtd = parmXtd;

        if(parser.has_pt3) {

            if( ! rom.build(&compilerPT3, outputFilename) ) {
                rom.error();
                printf("ERROR: ROM building error\n");
                return 1;
            }

            if(parmSymbols) {
                SaveSymbolFile(&compilerPT3, rom.code_start);
            }

        } else {

            if( ! rom.build(&compiler, outputFilename) ) {
                rom.error();
                printf("ERROR: ROM building error\n");
                return 1;
            }

            if(parmSymbols) {
                SaveSymbolFile(&compiler, rom.code_start);
            }

        }

        // finish process

        if(! parmQuiet) {

            if(parser.has_pt3) {

                if(compilerPT3.fileList.size()) {
                    int i, t = compilerPT3.fileList.size(), len1 = 0, len2 = 0;
                    for(i = 0; i < t; i++) {
                        len1 += compilerPT3.fileList[i]->length;
                        len2 += compilerPT3.fileList[i]->packed_length;
                    }
                    printf("%i file(s) attached into ROM = %i byte(s) [%i unpacked]\n", t, len2, len1);
                }
                if(compilerPT3.resourceList.size()) {
                    printf("%i resource(s) found\n", (int) compilerPT3.resourceList.size());
                }
                if(compilerPT3.font) {
                    printf("Built-In fonts included as resources\n");
                }

                if(parmXtd) {
                    printf("Extended memory scheme mode activated (Konami with SCC mapper).\n");
                    printf("MegaROM size = %ikb\n", compilerPT3.segm_total * 8);
                    printf("Resources+Compiled code occupied %.1f%% of MegaROM space\n",  rom.stdMemoryPerc);
                } else {
                    printf("Standard memory scheme mode activated.\n");
                    printf("Resources+Compiled code occupied %.1f%% of avaliable space\n",  rom.stdMemoryPerc);
                }

                printf("RAM memory will be occupied in %.1f%% of avaliable space\n",  compilerPT3.ramMemoryPerc);

                if(parmSymbols) {
                    printf("Symbols file created for use on OpenMSX debugger\n");
                }

                printf("Conversion finished with success.\n");

                printf("\nIncluded into this ROM:\n");
                printf("- XBASIC support routines, copyright by J.Suzuki 1989.\n");
                printf("- Pletter v0.5c1 by XL2S Entertainment 2008.\n");
                if(compilerPT3.pt3) {
                    printf("- PT3 player for MSX by Alfonso D. C. aka Dioniso <dioniso072@yahoo.es>\n");
                }

                printf("%s\n", info_support);

            } else {

                if(compiler.fileList.size()) {
                    int i, t = compiler.fileList.size(), len1 = 0, len2 = 0;
                    for(i = 0; i < t; i++) {
                        len1 += compiler.fileList[i]->length;
                        len2 += compiler.fileList[i]->packed_length;
                    }
                    printf("%i file(s) attached into ROM = %i byte(s) [%i unpacked]\n", t, len2, len1);
                }
                if(compiler.resourceList.size()) {
                    printf("%i resource(s) found\n", (int) compiler.resourceList.size());
                }
                if(compiler.font) {
                    printf("Built-In fonts included as resources\n");
                }

                if(parmXtd) {
                    printf("Extended memory scheme mode activated (Konami with SCC mapper).\n");
                    printf("MegaROM size = %ikb\n", compiler.segm_total * 8);
                    if(compiler.resourceList.size()) {
                        printf("Resources occupied %.1f%% of MegaROM space\n",  rom.rscMemoryPerc);
                    }
                    printf("Compiled code occupied %.1f%% of MegaROM space\n",  rom.stdMemoryPerc);
                } else {
                    printf("Standard memory scheme mode activated.\n");
                    if(compiler.resourceList.size()) {
                        printf("Resources occupied %.1f%% of avaliable space\n",  rom.rscMemoryPerc);
                    }
                    printf("Compiled code occupied %.1f%% of avaliable space\n",  rom.stdMemoryPerc);
                }

                printf("RAM memory will be occupied in %.1f%% of avaliable space\n",  compiler.ramMemoryPerc);

                if(parmSymbols) {
                    printf("Symbols file created for use on OpenMSX debugger\n");
                }

                printf("Conversion finished with success.\n");

                printf("\nIncluded into this ROM:\n");
                printf("- XBASIC support routines, copyright by J.Suzuki 1989.\n");
                printf("- Pletter v0.5c1 by XL2S Entertainment 2008.\n");
                if(compiler.akm) {
                    printf("- Arkos Tracker 2 minimalist player v2.0.1 by Julien Névo, 2021\n");
                }
                if(compiler.has_tiny_sprite) {
                    printf("- Tiny Sprite resource file support (Rafael Jannone, 2022)\n");
                }

                printf("%s\n", info_support);

            }

        }

    } else {

        // TOKEN ANALYSIS
        // default basic interpreter tokenizer

        if(! parmQuiet)
            printf("(2) Doing token analysis...\n");

        tokenizer.debug = parmDebug;
        tokenizer.force_turbo = parmTurbo;

        if( ! tokenizer.evaluate(&lexer) ) {
            tokenizer.error();
            printf("ERROR: Syntax error at line %i\n", tokenizer.lineNo);
            return 1;
        }

        if(tokenizer.turbo_mode)
            parmTurbo = true;

        if(! parmQuiet) {
            if(parmTurbo)
                printf("Turbo mode detected and activated\n");
            if(tokenizer.cmd)
                printf("CMD instruction detected and activated\n");
            if(tokenizer.pt3)
                printf("PT3 support detected and activated\n");
        }

        if( parmDebug ) {
            printf("Displaying token analysis:\n");
            tokenizer.print();
        }

        // ROM OUTPUT

        if(! parmQuiet)
            printf("(3) Building ROM...\n");

        rom.turbo = parmTurbo;
        rom.xtd = parmXtd;
        rom.stripRemLines = !parmNoStripRemLines;

        if( !parmQuiet && parmNoStripRemLines ) {
            printf("Including remark (REM) lines into ROM file\n");
        }

        if( ! rom.build(&tokenizer, outputFilename) ) {
            rom.error();
            printf("ERROR: ROM building error\n");
            return 1;
        }

        // finish process

        if(! parmQuiet) {
            if(tokenizer.resourceList.size()) {
                printf("%i resource(s) found\n", (int) tokenizer.resourceList.size());
            }
            if(tokenizer.font) {
                printf("Built-In fonts included as resources\n");
            }

            if(parmXtd) {
                printf("Extended memory scheme mode activated.\n");
                printf("Basic code occupied %.1f%% of STD avaliable space\n",  rom.stdMemoryPerc);
            } else {
                printf("Standard memory scheme mode activated.\n");
                if(tokenizer.resourceList.size())
                    printf("Basic code+resources occupied %.1f%% of avaliable space\n",  rom.stdMemoryPerc);
                else
                    printf("Basic code occupied %.1f%% of avaliable space\n",  rom.stdMemoryPerc);
            }

            printf("Conversion finished with success.\n");

            if(tokenizer.turbo_mode || parmTurbo|| tokenizer.cmd || tokenizer.pt3) {
                printf("\nIncluded into this ROM:\n");
                if(tokenizer.turbo_mode || parmTurbo) {
                    printf("- XBASIC, copyright by J.Suzuki 1989.\n");
                }
                if(tokenizer.cmd) {
                    printf("- Pletter v0.5c1 by XL2S Entertainment 2008.\n");
                    printf("- VRAM Depacker v1.1 by Metalion 2008 (metalion@orange.fr).\n");
                }
                if(tokenizer.pt3) {
                    printf("- PT3 player for MSX by Alfonso D. C. aka Dioniso <dioniso072@yahoo.es>\n");
                }
            }

            printf("%s\n", info_support);

        }

    }

    return 0;
}

bool FileExists(char *filename) {
    FILE *file;
    if ((file = fopen(filename, "r"))) {
        fclose(file);
        return true;
    }
    return false;
}

bool SaveSymbolFile(Compiler *compiler, int code_start) {
    FILE *file;
    CodeNode *codeItem;
    int i, t;
    char s[255];

    if ((file = fopen(symbolFilename, "w"))) {

        strcpy(s, "LOADER EQU 04010H\n");
        fwrite(s, 1, strlen(s), file);

        t = compiler->codeList.size();

        for(i = 0; i < t; i++) {
            codeItem = compiler->codeList[i];
            if(codeItem->debug) {
                sprintf(s, "%s EQU 0%XH\n", codeItem->name.c_str(), codeItem->start+code_start);
                fwrite(s, 1, strlen(s), file);
            }
        }

        t = compiler->dataList.size();

        for(i = 0; i < t; i++) {
            codeItem = compiler->dataList[i];
            if(codeItem->debug) {
                sprintf(s, "%s EQU 0%XH\n", codeItem->name.c_str(), codeItem->start+code_start);
                fwrite(s, 1, strlen(s), file);
            }
        }

        fclose(file);
        return true;
    }
    return false;
}


bool SaveSymbolFile(CompilerPT3 *compiler, int code_start) {
    FILE *file;
    CodeNode *codeItem;
    int i, t;
    char s[255];

    if ((file = fopen(symbolFilename, "w"))) {

        strcpy(s, "LOADER EQU 04010H\n");
        fwrite(s, 1, strlen(s), file);

        t = compiler->codeList.size();

        for(i = 0; i < t; i++) {
            codeItem = compiler->codeList[i];
            if(codeItem->debug) {
                sprintf(s, "%s EQU 0%XH\n", codeItem->name.c_str(), codeItem->start+code_start);
                fwrite(s, 1, strlen(s), file);
            }
        }

        t = compiler->dataList.size();

        for(i = 0; i < t; i++) {
            codeItem = compiler->dataList[i];
            if(codeItem->debug) {
                sprintf(s, "%s EQU 0%XH\n", codeItem->name.c_str(), codeItem->start+code_start);
                fwrite(s, 1, strlen(s), file);
            }
        }

        fclose(file);
        return true;
    }
    return false;
}
