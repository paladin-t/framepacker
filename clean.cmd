@echo off
echo Cleaning, a moment please...

attrib *.suo -s -r -h
del /f /s /q *.suo
del /f /s /q *.opensdf
del /f /s /q *.sdf

rd /s /q output
rd /s /q temp

echo Cleaning done!
