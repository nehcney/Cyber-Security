A program that indexes and searches security telemetry log files for threat indicators to discover new malicious websites and software files.
--------------------------------------------------------------------------------------------------
In this project I created a set of classes that would ingest data from telemetry log files, organize them into a pair of disk-based multimap hash tables, and then search through those data structures to discover new threat indicators using an algorithm that analyzes new entities and their relationship to known threat indicators, as well as the prevalence of that entity within the compiled data.

The classes are:
- BinaryFile
- DiskMultiMap
- IntelWeb

Descriptions of these classes and how they operate are documented in their respective header and cpp files. As a general overview, BinaryFile is a class that aids in file I/O, DiskMultiMap is a disk-based multimap hash table, and IntelWeb is responsible for ingesting data from the telemetry files, organizing the data, searching through the data, and discovering new malicious entities.

As the specs for the project are quite extensive I've included a pdf with the full specs written out (spec.pdf).

Copyright (c) 2016 Yen Chen
