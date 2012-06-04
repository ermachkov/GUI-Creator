#!/bin/bash

cd ../dmb
find . -type f \! -name CMakeLists.txt.user -exec cp --parents '{}' ../GUI-Creator \;
cd ../GUI-Creator

sed -i 's/dmb/gui-creator/' CMakeLists.txt
sed -i 's/dmb/gui-creator/' src/main.cpp

git add .
git commit -m "Commit from: `date`"
git push
