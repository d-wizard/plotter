# Copyright 2015, 2019, 2021 Dan Williams. All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
# to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# 
import os
import sys
import string
import time

scriptDir = os.path.dirname(os.path.realpath(__file__))

SVN_PATH = '"c:/Program Files/TortoiseSVN/bin/svn.exe"' if sys.platform == 'win32' else 'svn'

REV_DELIM = 'Revision: '

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

FILE_PATH = SCRIPT_DIR + os.sep + 'revDateStamp.h'

def readWholeFile(path):
   try:
      fileId = open(path, 'r')
      retVal = fileId.read()
      fileId.close()
   except:
      retVal = ""
   return retVal
   
def writeWholeFile(path, fileText):
   fileId = open(path, 'w')
   fileId.write(fileText)
   fileId.close()

def getSvnVersion():
   cwd = os.getcwd()
   os.chdir(SCRIPT_DIR)

   retVal = 0
   
   # Set CWD to the path this script is in
   origCwd = os.getcwd()
   os.chdir(scriptDir)
   
   svnVersionCmd = SVN_PATH + ' info'

   # Run the 'info' command and loop over the results until the revision information line is found.
   p = os.popen(svnVersionCmd, "r")
   while 1:
      line = p.readline()

      if not line: break
      
      if line.find(REV_DELIM) >= 0:
         retVal = int(line.split(REV_DELIM)[1])
         break

   os.chdir(cwd)
   return retVal
       
svnRev = getSvnVersion()
time.ctime()
dateStamp = time.strftime('%m/%d/%y')

revDateInfo = 'Rev: ' + str(svnRev) + " - Date: " + dateStamp

fileText = '#define REV_DATE_STAMP "' + revDateInfo + '"'

if fileText != readWholeFile(FILE_PATH):
   writeWholeFile(FILE_PATH, fileText)
