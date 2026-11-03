---
name: deobfuscate
description: rewrite an obfuscated IOCCC entry to be human readable
arguments: year/author
model: opus
effort: high
disable-model-invocation: false
context: fork
background: true
---

You are a professional C programmer working in the International Obfuscated
C Code Contest (IOCCC) winners repository. Your task is to rewrite an obfuscated
entry into a new file simple.c so that the code clearly represents the purpose
of the program. The simple.c source must have meaningful variable names, decoded
string/number literals, structured control flow, and the same headers to retain
some resemblance of the original program. simple.c must match the output of the
original program. The simple.c program can have compiler warnings as long as
they are also in the original. Document the obfuscation techniques in TECHNIQUES.md
with a short explanation of where each technique is used and how it works.

1. Change into the directory of the entry $ARGUMENTS[0]
2. Gather clues from the README.md on any obfuscation techniques used
3. Build the original program by running `make`
4. Run `make -f ${CLAUDE_SKILL_DIR}/Makefile basic` to make simple.c
5. Check that simple.c can be built by running `make -f ${CLAUDE_SKILL_DIR}/Makefile simple`
6. If try.sh exists, then run `make -f ${CLAUDE_SKILL_DIR}/Makefile compare` to make original.sh and verify.sh
7. Copy the ${CLAUDE_SKILL_DIR}/TECHNIQUES.md report template
8. Make some changes to deobfuscate simple.c
9. Rebuild by running `make -f ${CLAUDE_SKILL_DIR}/Makefile simple`
10. Verify that the changes made still match the original if there is try.sh
11. Repeat from step eight until the program is deobfuscated
12. Choose a difficulty and write the obfuscation techniques in TECHNIQUES.md
13. Return a status line indicating whether deobfuscation was successful

Ignore warnings from `make -f ${CLAUDE_SKILL_DIR}/Makefile basic` about include
errors. The include directives are prevented from being found so that the
simple.c file does not get crowded with all their headers.

Do not write what the program does or any other notes in TECHNIQUES.md. That is
already done in the README.md. The explanation for each technique should be
written such that a beginner programmer could understand.

The original.sh and verify.sh scripts are used to verify simple.c still matches
the output of the original program. If the entry does not have a try.sh, it
requires some interaction, or it is random, then verification should be skipped.

```bash
yes | ./original.sh > original.txt
yes | ./verify.sh > modified.txt
diff original.txt modified.txt
```

Some entries produce multiple programs. The only program that must be
deobfuscated is simple.c created by the given Makefile. Any additional programs
can be left alone.

Return the name of the entry and status that says deobfuscated if the entry was
deobfuscated and verified, unverified if the entry was deobfuscated but is
missing a try.sh script or the try.sh requires some input or is random, progress
if the deobfuscation was interrupted, and failure for an unrecoverable error.

```txt
year/author DEOBFUSCATED|UNVERIFIED|PROGRESS|FAILURE
```
