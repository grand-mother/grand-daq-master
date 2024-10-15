# Create HTML coverage report with lcov

## Install lcov

```bash
sudo apt install lcov
```

## Creation procedure 

* compile test project with option : -fprofile-arcs -ftest-coverage -g
* run test
* create file raw coverage with gcov for each module
* create rapport with lcov
* create HTML with genhtml

```bash
cd /path/tests
scons -f makefile_scons.py -c ; scons -f makefile_scons.py
run_cpputest 

cd ../myfiles/
gcov *.c
rm rapport.info
lcov --directory ./ -c -o rapport.info
rm -rf ../rapport
genhtml -o ../rapport -t "couverture de code des tests" rapport.info

```

open index.html in  path/tests/rapport  

