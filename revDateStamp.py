# Copyright 2015, 2019, 2021 - 2022 Dan Williams. All Rights Reserved.
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

GIT_PATH = '"C:/Program Files/Git/bin/git.exe"' if sys.platform == 'win32' else 'git'

COMMIT_DELIM = 'commit '

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

def getGitCommit():
   cwd = os.getcwd()
   os.chdir(SCRIPT_DIR)

   retVal = 0
   
   # Set CWD to the path this script is in
   origCwd = os.getcwd()
   os.chdir(scriptDir)
   
   gitVersionCmd = GIT_PATH + ' log -1'

   # Run the 'info' command and loop over the results until the revision information line is found.
   p = os.popen(gitVersionCmd, "r")
   while 1:
      line = p.readline()

      if not line: break
      
      if line.find(COMMIT_DELIM) >= 0:
         retVal = line.split(COMMIT_DELIM)[1]
         if retVal.find(' ') > 0:
            retVal = retVal.split(' ')[0]
         if len(retVal) > 7: # 7 characters of the hash seems to be enough
            retVal = retVal[:7]
         break

   os.chdir(cwd)
   return retVal
       
gitCommit = getGitCommit()
time.ctime()
dateStamp = time.strftime('%m/%d/%y')

revDateInfo = 'Date: ' + dateStamp + " - Commit: " + gitCommit

fileText = '#define REV_DATE_STAMP "' + revDateInfo + '"'

if fileText != readWholeFile(FILE_PATH):
   writeWholeFile(FILE_PATH, fileText)
