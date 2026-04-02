GitLite: A Simplified Version Control System
CS2001 Data Structures | Fall 2024 | FAST NUCES Islamabad
Team: Abubakar (23i-0515) · Abbas Raza (23i-0733) · Abdullah Bin Nasir (23i-0803)

What is GitLite?
GitLite is a file-backed, command-line version control system built in C++. It mimics core Git workflows — branching, committing, merging, and history — while storing all data in real files on disk using self-balancing tree structures. Every record in a CSV dataset becomes a node in the tree, persisted as its own file, ensuring no data lives solely in RAM.
The project was built to demonstrate AVL Trees, Red-Black Trees, and B-Trees in a practical setting, with Merkle Trees providing cryptographic-style data integrity verification across branches.

Features

Three Tree Backends — Interchangeable AVL, Red-Black, and B-Tree structures.
File-Backed Storage — Each tree node is a separate file on disk, optimizing memory for large datasets.
Git-like Commands — Full suite including init, commit, branch, checkout, merge, and log.
Merkle Tree Integrity — Recomputes hashes to detect data corruption or unauthorized manual edits.
Instructor Hash — Custom key generation using digit/ASCII products modulo 29.
Auto-Resume — Automatically picks up the last saved repository state on launch.
CSV Support — Supports custom datasets with column-selectable key indexing.


Prerequisites & Build
Requirements

Compiler: GCC 9+ or MSVC 2019+ (C++17 support required)
Standard Library: <filesystem>
Dataset: A CSV file (e.g. healthcare_dataset.csv) placed in the root directory


Download the dataset from Kaggle: Healthcare Dataset and place healthcare_dataset.csv in the same directory as the executable before running.

Compile
Windows (MinGW):
bashg++ -std=c++17 -O2 -o GitLite GitLite.cpp
Linux / macOS:
bashg++ -std=c++17 -O2 -o GitLite GitLite.cpp -lstdc++fs
Run
bash./GitLite        # Linux / macOS
GitLite.exe      # Windows
If a repository was previously saved, GitLite resumes it automatically. Otherwise it greets you with the help prompt.

Quick Start

Launch GitLite and initialize with your dataset:

bashgitlite > init healthcare_dataset.csv

Select your configuration when prompted — key column, tree type (1 = AVL, 2 = Red-Black, 3 = B-Tree), and row limit (-1 loads all rows).
GitLite builds the tree and drops you into the REPL:

Done. Branch 'main' | Tree: AVL | Key: Age | Rows: 10000

gitlite (main) >

Start versioning:

bashgitlite (main) > commit First load
gitlite (main) > branch feature-1
gitlite (main) > checkout feature-1
gitlite (feature-1) > search 30
gitlite (feature-1) > visualize-tree
gitlite (feature-1) > verify

Command Reference
Repository & Data
CommandDescriptioninit <csv>Initialize a repo; prompts for tree type and key columnsaveFlush current repository state and metadata to diskload <branch>Load a previously saved branch by nameverifyRecompute Merkle root to check data integrityadd <row>Insert a new CSV-formatted recorddelete <key>Delete a record by key value (AVL only)search <key>Find and display a record by its keyupdate <key> <col> <val>Update a specific column of a recordprintPrint all records in sorted ordervisualize-treeDisplay an ASCII diagram of the active tree
Version Control
CommandDescriptioncommit <message>Snapshot current state with a Merkle hash and timestampbranch <name>Create a new branch stored in a separate foldercheckout <name>Switch active branch and load its tree structurebranchesList all branches; active branch marked with *current-branchPrint the name of the currently active branchdelete-branch <name>Permanently delete a branch and its foldermerge <src> <dst>Copy node files from source branch into destinationlogView full commit history for the active branch

Tree Structures
All three trees use a file-backed model where each node is a .node file containing the CSV row data, left/right child filenames, parent filename, height or color, and a Merkle hash.
TreeComplexityNotesAVL TreeO(log n)Self-balancing BST. Supports insert, delete, and search via LL, RR, LR, RL rotations.Red-Black TreeO(log n)Balanced via recoloring and rotations. High insertion efficiency. Delete not yet implemented.B-Tree (order t)O(log_t n)Multi-key nodes; grows upward via splitting. Best suited for large datasets.
Node File Format (AVL / Red-Black)
Line 0 : CSV row data
Line 1 : left child filename  (NULL if none)
Line 2 : right child filename (NULL if none)
Line 3 : parent filename      (ROOT if root node)
Line 4 : height (AVL) or color R/B (Red-Black)
Line 5 : Merkle hash (64-char hex)
Node File Format (B-Tree)
Line 0      : numKeys
Lines 1..n  : key<TAB>row_data
Line n+1    : isLeaf (1 = leaf, 0 = internal)
Lines n+2.. : child filenames (numKeys+1 entries if not a leaf)

Data Integrity via Merkle Trees
Every commit records a 64-character Merkle root hash computed by combining child hashes with node data in post-order. The hash function uses FNV-1a mixing over four 64-bit accumulators to produce a deterministic hex string.
The verify command recomputes the current Merkle root and compares it against the last committed hash. A mismatch means data was altered after the last commit — either by a direct file edit or corruption.
gitlite (main) > verify
Computing Merkle root...
Last commit: 59c62f6c93214e5d405a42d69c5e8f2a...
Current:     59c62f6c93214e5d405a42d69c5e8f2a...
OK - data integrity verified.

Repository Layout on Disk
GitLite.cpp                  <- source file
healthcare_dataset.csv       <- dataset (not tracked by git)
.gitignore

gitlite_main/                <- branch folder for 'main'
  META.gitlite               <- metadata (tree type, root, row count, etc.)
  COMMITS.log                <- commit history with Merkle hashes
  Bobby_JacksOn_0.node       <- individual tree node files
  LesLie_TErRy_1.node
  ...

gitlite_feature-1/           <- branch folder for 'feature-1'
  META.gitlite
  COMMITS.log
  ...

.gitignore
Make sure to include the following in your .gitignore so the generated node files are not tracked:
gitlite_*/
*.node
META.gitlite
COMMITS.log
*.csv
*.exe
*.o
*.out
.vscode/

Known Limitations

Delete is only implemented for AVL trees. Red-Black and B-Tree deletion is not yet supported.
Update on Red-Black and B-Tree inserts an updated record but does not remove the old one.
Merge is a file-copy strategy — it does not perform a semantic 3-way merge.
The instructor hash maps values into a small range (mod 29), so hash collisions are common. Row number is used as a tiebreaker in filenames.


Authors
Name               Roll Number   Contribution
Abdullah Bin Nasir 23i-0803      AVL Tree, CLI, Merkle Tree
Abbas Raza         23i-0733      Red-Black Tree, Branching Logic
Abubakr            23i-0515      B-Tree, File I/O

License
This project is submitted as academic coursework for CS2001 at FAST NUCES Islamabad. Unauthorized submission of this code as your own work in any academic context constitutes plagiarism.
