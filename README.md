# THC
Tagged Hash Code

Hash codes can be created in many different ways from file contents, but there is always a risk of collisions occurring when a hash function returns the same value for different file contents. Although that is rare with a good hash function, the risk of collisions can be mitigated by 'tagging' a hash code with file metadata such as the file size. The "thc" utility uses  [xxHash] (https://github.com/Cyan4973/xxHash) to create and check lists of 'tagged' hash codes.

```
usage:
        thc [-r][-d][-f][-i][-c THC-list] files
where:
        -r = recurse through folders
        -d = delete files with matching hash code, filessize, date and time of modification, and filename
        -f = ignore date and time of modification, and filename when matching tagged hash codes
        -i = ignore case of filename
        -c = compare matching file in THC-list

example:
        thc Makefile
        8a7cf9c83a87c3b4          723 2024/08/05 16:43:49 Makefile

        thc -r .
        4170b8fdf72bf7ff         4260 2024/04/08 19:31:56 ./stamp.c
        65d2ca6831b052be          403 2024/04/08 20:11:14 ./fco
        46251f819e07ff98          550 2024/04/10 14:28:15 ./rmdups
        79202a1901706455          638 2024/04/10 14:28:43 ./dups
        0afe0f2d77e69562          640 2016/10/11 17:14:01 ./fdiff
        2aa895ac33734b3f          554 2024/04/10 00:25:57 ./flocate
        e3741b2d23e42801          286 2015/11/25 15:15:34 ./fci
        045506a510452e09        15664 2024/08/05 02:22:11 ./thc.c
        8a7cf9c83a87c3b4          723 2024/08/05 16:43:49 ./Makefile
        335a1a3aa9045070          238 2024/04/10 00:14:11 ./thc-format
        0000000000000000            0 2024/04/10 00:14:11 . (directory)

usage:
        thcc [-d] [-v] [directory] < THC-list
where:
        -d = delete files with matching hash code, filessize, date and time of modification, and filename
        -v = verbose output

example:
        thc -r . >/var/tmp/files.thc
        thcc < /var/tmp/files.thc
        thcc -v < /var/tmp/files.thc
        4170b8fdf72bf7ff         4260 ./stamp.c                        OK: (file)
        65d2ca6831b052be          403 ./fco                            OK: (file)
        46251f819e07ff98          550 ./rmdups                         OK: (file)
        79202a1901706455          638 ./dups                           OK: (file)
        0afe0f2d77e69562          640 ./fdiff                          OK: (file)
        2aa895ac33734b3f          554 ./flocate                        OK: (file)
        e3741b2d23e42801          286 ./fci                            OK: (file)
        045506a510452e09        15664 ./thc.c                          OK: (file)
        8a7cf9c83a87c3b4          723 ./Makefile                       OK: (file)
        335a1a3aa9045070          238 ./thc-format                     OK: (file)
        0000000000000000            0 . (directory)                    OK: (directory)
```
Also included is a simple file-based RCS*
```
usage:
        fci file
where:
        a copy of file is checked into a .old folder and stamped with a time and date suffix

usage:
        fco file
where:
        file is the basename of a time and date stamped file to check out of a .old folder

usage:
        fdiff file
where:
        file is compared to a time and date stamped file with the same basename in a .old folder

usage:
        flocate [-c] file
        note: requires root permission to update the locate/plocate database
where:
        file is the basename of files found using "locate" ignoring a time and date suffix
        -c = run "thc" on the files located

usage:
        dups [directories]
        find dupicate files in one or more directories

usage:
        rmdups [directories]
        remove files duplicated in two or more directories

usage:
        stamp [-pruv] files
where:
        -p = time and date prefix (defalt = suffix)
        -r = stamp files recursively
        -u = unstamp files
        -v = verbose
```
*disclaimer: These utilities are useful for detecting and removing duplicate files, but should be used with CAUTION!


