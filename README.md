# File-System
## Question 1
Objectives :
* Understand the problems with contiguous allocation and the need for alternative file
structures.
* Implement and compare different file structures for managing files in a file system.
* Evaluate the performance and scalability of the file system with different file structures.

Requirements :
* Implement a file system that supports three different file structures for managing files:<br>
➢ Contiguous allocation: each file is allocated a contiguous block of disk space.<br>
➢ Linked allocation: each file is allocated a linked list of disk blocks.<br>
➢ Indexed allocation: each file is allocated an index block that contains pointers to the disk
blocks that store the file's data.<br>
➢ Modified contiguous allocation: each file is allocated an initial contiguous area of a
specified size, and overflow areas are allocated as needed and linked to the initial area.
* Implement file operations for creating, reading, updating, and deleting files, using the
different file structures.
* Measure and compare the performance of the file system with the different file structures, in
terms of space utilization, file access time.
