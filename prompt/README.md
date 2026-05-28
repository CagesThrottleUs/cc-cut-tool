This project's main goal is to create cut tool in C++ and I want to use AI and my harness to create it.

The way that I have designed it on my notebook is simple to support the following pattern directly:

```
SYNOPSIS
     cut -b list [-n] [file ...]
     cut -c list [file ...]
     cut -f list [-d delim] [-s] [file ...]
```


To achieve this I believe I would like you to use the appropriate skills to create a proper industry acceptable code for the tool using brainstorming and specs properly. 

This would also test against the Harness if we actually have an extremely well written industry standard code or not.

Please ensure that we are using proper low-level and high-level design patterns.

Step Parse Arguments and keep inside a proper structure:

1. Mode - probably an enum that supports: BYTE, CHARACTER, FIELD. It would detect from argv[1]
2. List Detection - this would probably use the following pattern:

```python
def parse_args(argc, argv):
    mode = detect_mode(argv[1])
    index = 2
    if argv[1].length() > 2:
        parse_list(argv[1].substr(2))
        index = 2
    else:
        parse_list(argv[2])
        index = 3
    getProperties(args, index, mode)
    parse_files(argv, index + 1) # last index
```

The parse list would need to ensure that first we check if it has any "," characters present in it - if yes we would split on that immediately, else use whitespace as a skip to get all the characters as a list.

These list would then be need to be interpreted in a way and added to a set that would be used to determine which index to use and print the result as. 

Direct numbers would be added to the set as is (after doing -1 to it)
Ranges can be shown as N-M, we would need to add all the numbers in between to the set after doing -1 to it
Ranges with -M, would mean we have to add all numbers from 1 to M to the set after doing -1 to it
Ranges with N- would require a special arrangement so that it can be understood that we have an infinite set of numbers from N-1 to the end of the list would always need to be considered

3. Properties detection would involve that if mode is BYTE, then `-n` would provide:
```text
 -n      Do not split multi-byte characters.  Characters will only be output if at least one byte is selected, and, after a prefix of zero or more
             unselected bytes, the rest of the bytes that form the character are selected.
```
In case it is character -> no-op
In case of field -> check if delimiter provided else defaults to whitespace characters to split on and if `-s` is provided, then suppress non-delimited lines:
```text
-s      Suppress lines with no field delimiter characters.  Unless specified, lines with no delimiters are passed through unmodified.

```

4. The Parse files process would involve taking in everything from index + 1 to end of it. If none specified (index + 1 >= argc) then file interface receives stdin, if 1 is total number of files specified: if it was '-' then file interface is stdin, else it was the path to the file. If > 1 then essentially we would need to de-duplicate here as well (user specifies two '-' or same file) but the output as expected in case of one stands.

What I expect from the file interface? It needs to support both stdin and file paths.

In case of stdin, everything is in memory for file if >= 100 MB we would want to use MMIO (Boost as a dependency), else load everything into memory and then process.

File interface needs to support load(), getline() functionality that would be used in algorithm to process the file.

Algorithm in question:

```python
for file in files:
    file.load()
    while True:
        line = file.getline()
        if not line:
            break
        values = get_fields(line, args)  # My guess strategy here
        print_values(values, args)  # Reuse the delimiter and from mode to print values
```

Let's start the implementation with specs and everything.