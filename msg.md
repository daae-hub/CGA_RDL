2020.11.25 [-b 201125/emailProblem] not process well about email address

Solve #52
guess : perl process @ as array.
so i change @ to special string before processing.  then i recover it when i show the result

	modified:   2_replace.pl
	modified:   perllib/MY/CHARLES.pm

-------------------------------------------
2020.07.30 [-b master] v1.0.4 skip the copy when host is unreachable.

- ping -W 1 -c 1 host 
- check return value ( 0 is successful)

-------------------------------------------
2020.07.26 [-b master] v1.0.4 I think this is final update to support sub functions for each module (ex. TIDL) 

- refer to : https://stackoverflow.com/questions/1712016/how-do-i-include-functions-from-another-file-in-my-perl-script
```
$ cat m1.pl 
use strict;
sub x { warn "aard"; }
1;

$ cat m2.pl 
use strict;
require "m1.pl";
x();

$ perl m2.pl 
aard at m1.pl line 2.
```

- we do not chnange anymore to support special function for TIDL
    - if TIDL has yours.pl ,  you add --yours=yours.pl after 2_replace.pl
- add option : --yours
perlscript=2_replace.pl
Help :
	--inputstci=[stcI or stc file]
		  default input stc or stcI file name : 
	--outputdb=[output file]
		  default output DB file name : 
	--cga_rdl_version_input=[version file name]
		  default input version file name : 
		  if null , we ignore the version between excel version input file and [VARIABLE]Excel_Version  in excel file ()
	--debug		  debug mode : ITERATE  IFEQUAL IFNOTEQUAL
	--debugdetail		  --debug + ITERATE detail 
		  debug mode : ITERATE  IFEQUAL IFNOTEQUAL  ITERATE detail 
	--original		  run NO performance mode
	--nolog		  print log into /dev/null
	--yours=[module file name]
		  default is null (not use it)
	--help

-------------------------------------------
2020.07.14 [-b master] v1.0.4 solve #45 : -o StrictHostKeyChecking=no passes the qeustion yes/no when we connect the host at first.

- 1_csv.pl  1_excel.pl
    - solve #45 : -o StrictHostKeyChecking=no passes the qeustion yes/no when we connect the host at first.
- 2_replace.pl
    - add getFunctionName("abc(a,b,c)")   => abc
    - add getInitArg("a(a,b,c)" ) =>  0,0,0


-------------------------------------------
2020.07.14 [-b master] v1.0.4 solve #45 : backup inputfile (csv or excel) to warehouse

# solve #45 : backup inputfile (csv or excel) to warehouse

## change options
```
perl 1_csv.pl --help
Help :
        --input=[input excel file]
                  default input file name : ./1_example.csv
        --excel_version_input=[excel version input file]
                  default excel version of input file name :
                  if null , we ignore the version between excel version input file and [VARIABLE]Excel_Version  in excel file (./1_example.csv)
        --output=[output file with global variables]
                  default output file name : ./default.GV
        --csv_out=[csv output file]
                  default csv output file name : ./1_example.csv
        --python
                  run command with python scripti to backup
        --destinfo=[input file name with info]
                  default : input_file_name
        --id=[host id ]
                  default : tidl
        --passwd=[passwd of id]
                  default : tidl1234
        --host=[host name]
                  default : abc.com
        --location=[warehouse location]
                  default : ~/warehouse/
        --help

```

- `perl 1_csv.pl --input=./time/Code_Generator.csv --csv_out=./time/o.csv --host=lotto645.abc.com`
```
sshpass -p 'tidl1234' scp ./time/Code_Generator.csv tidl@lotto645.abc.com:~/warehouse/2020-07-14-13-15-38-cheoljoo.lee-Time-v2.9.5-Code_Generator.csv START
sshpass -p 'tidl1234' scp ./time/Code_Generator.csv tidl@lotto645.abc.com:~/warehouse/2020-07-14-13-15-38-cheoljoo.lee-Time-v2.9.5-Code_Generator.csv END

```


-------------------------------------------
2020.07.14 [-b master] v1.0.4 solve #43 : Automatic fill out the empty cell with proper value 

# solve #43  : we wanna have the same result bwtween Origiand and After table.

## Original  : we should fill out completely

|  [HEADER]A | [HEADER]B | value |
|-------|-----|----|
| receiver1 | A | One |
| receiver1 | B | Two |

## After : Automatics fill out the form

|  [HEADER]A | [HEADER]B | value |
|-------|-----|----|
| receiver1 | A | One |
| | B | Two |

-------------------------------------------

add in($a,$b) sub-function : is $b in $a?

200424--------------
- add in sub-function
- IFEQUAL("+<+in($modulename,tidl)+>+","")

200401--------------
you can run with  csv file instead of xlsx file.
First of all , you convert the file into csv file when you have error of perl package for excel.
if you use MAC OSX , your csv file will have as one line with ^M. so you can replace ^M with \r\n

--------------
support sldd & %+<+$API+>+ and restore $g_y policy (plus)

v1.0.4
- support sldd 
- support ```ITERATE %+<+$API+>+ ``` syntax
    - add ```$iterate_var_name = replace_var_with_value($iterate_var_name);```
- restore $g_y policy (plus)
    - replace_var_with_value does not break becasue break contents to improve speed  is impossible to keep the order when we count with plus().

