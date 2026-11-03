---
name: coordinate
description: deobfuscate all the entries for a year in the IOCCC
arguments: year
model: sonnet
effort: medium
---

You are a project manager tasked with assigning a team of subagents to
deobfuscate all the IOCCC entries in $ARGUMENTS[0]. The deobfuscation process
done by subagent must produce a deobfuscated simple.c file in the same directory
as the entry. If a PROGRESS.md file is not already in year directory, copy the
${CLAUDE_SKILL_DIR}/PROGRESS.md template to the directory for the year and fill
it with each author and their initial status should be not started (Q). Nothing
has to be done if the entry is marked as deobfuscated or unverified in
PROGRESS.md. The only writing in PROGRESS.md must be changing the year and
updating the status codes for each entry. The following should be done for each
entry in the year.

1. Invoke the deobfuscate skill passing year/author to spawn a subagent
2. Wait for the agent's output as a notification
3. Record the results returned by the subagent in PROGRESS.md
4. Record the difficulty from year/author/TECHNIQUES.md in the top level DIFFICULTIES.md
5. Commit changes for the entry using the message "deobfuscate year/author"

Subagents should be assigned to deobfuscate entries in batches of 3-5 at a time.
The agents will run in the background, do not start multiple agents for
deobfuscating a single entry. Only record a difficulty for the entry if the
result comes back from the subagent as deobfuscated or unverified.

The DIFFICULTIES.md file categorizes each entry according to its level of
obfuscation. An entry should be written as year/author and be in
year/alphabetical order under each difficulty category. The difficulty is
decided arbitrarily by the agent which deobfuscates the program.

The entry 1987/korn is already in DIFFICULTIES.md as an example.

Only commit the entry if the subagent returns as deobfuscated or unverified.

```bash
git add year/author year/PROGRESS.md DIFFICULTIES.md
git commit -m "deobfuscate year/author"
```

The subagent will return the name of the entry and a status that says
deobfuscated if the entry was deobfuscated and verified (D), unverified if the
entry could not be checked against the original program (E), progress if the
agent was interrupted (P), and failure for an error (F). If the agent does not
return a status or it is malformed, then assume progress.

```txt
year/author DEOBFUSCATED|UNVERIFIED|PROGRESS|FAILURE
```

The status codes in the PROGRESS.md file should be written like this example.

```
- [D] ferguson
```
