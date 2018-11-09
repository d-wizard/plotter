/* Copyright 2017 - 2018 Dan Williams. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <string>
#include "hist.h"
#include "dString.h"
#include "AutoDelimiter.h"

typedef DataHistogram<std::string> StringHistogram;
static const int VALID_DEC_CHAR[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, // + - .
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 0 to 9
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // E (scientific notation)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // e (scientific notation)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const int VALID_HEX_CHAR[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, // 0 to 9
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A to F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, // a to f
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

std::string getDelimiter(const std::string& t_dataToParse, bool isHex)
{
   StringHistogram mt_delimitHist;
   StringHistogram mt_delimitHistOneChar;
   const char* pc_data = t_dataToParse.c_str();
   char c_curChar = '\0';
   char c_prevChar = '\0';
   std::string t_curDelimit = "";
   int i_numChr = t_dataToParse.length();
   int i;
   int numBytesInDelimiter = 0;
   //const int* validCharTable = isHex ? VALID_HEX_CHAR: VALID_DEC_CHAR;

   for(i = 0; i < i_numChr; ++i)
   {
      c_curChar = pc_data[i];

      // Check for characters that should never be part of the delimiter.
      if( ( (c_curChar >= '0') && (c_curChar <= '9') ) ||
          ( (c_curChar >= 'a') && (c_curChar <= 'z') ) ||
          ( (c_curChar >= 'A') && (c_curChar <= 'Z') ) ||
          ( (c_curChar == '.') ) )
      /* Not sure why, but the if above seems to run quite a bit faster then the if below. I think the if below is better...
         if( validCharTable[(unsigned)c_curChar] ||
          (isHex && (c_curChar == 'X' || c_curChar == 'x')) )*/
      {
         if(numBytesInDelimiter)
         {
            if(numBytesInDelimiter > 1 && (c_prevChar == '-' || c_prevChar == '+') )
            {
               t_curDelimit = dString::Left(t_curDelimit, numBytesInDelimiter - 1);
            }
            mt_delimitHist.addNewEntry(t_curDelimit);
            mt_delimitHistOneChar.addNewEntry(dString::Left(t_curDelimit, 1));
            numBytesInDelimiter = 0;
            t_curDelimit = "";
         }
      }
      else
      {
         char ac_curChar[2] = {c_curChar, '\0'};
         t_curDelimit.append(ac_curChar);
         ++numBytesInDelimiter;
         c_prevChar = c_curChar;
      }
   }

   if(numBytesInDelimiter)
   {
      mt_delimitHist.addNewEntry(t_curDelimit);
      mt_delimitHistOneChar.addNewEntry(dString::Left(t_curDelimit, 1));
      numBytesInDelimiter = 0;
      t_curDelimit = "";
   }
   
   if(mt_delimitHistOneChar.getMaxEntryValue() == dString::Left(mt_delimitHist.getMaxEntryValue(), 1))
   {
      return mt_delimitHistOneChar.getMaxEntryValue();
   }
   else
   {
      return mt_delimitHist.getMaxEntryValue();
   }

    return "";
}

std::string removeNonDelimiterChars(const std::string& t_dataToParse, const std::string& t_delimit, bool isHex)
{
   char* pc_retVal = new char[t_dataToParse.size()+1]; // The return string can't be larger than the input string (make sure to allow for null terminator).
   int i_numValidChars = 0;

   const char* pc_data = t_dataToParse.c_str();
   int i_numChr = t_dataToParse.length();

   const char* pc_delim = t_delimit.c_str();
   int i_numDelimChr = t_delimit.length();

   const int* validCharTable = isHex ? VALID_HEX_CHAR: VALID_DEC_CHAR;

   for(int i = 0; i < i_numChr; ++i)
   {
      bool checkForDelim = (i + i_numDelimChr) <= i_numChr;
      bool isDelim = false;

      unsigned char c_curChar = pc_data[i];

      if(checkForDelim)
      {
         isDelim = memcmp(&pc_data[i], pc_delim, i_numDelimChr) == 0;
      }

      if(isDelim)
      {
         memcpy(pc_retVal+i_numValidChars, pc_delim, i_numDelimChr);
         i_numValidChars += i_numDelimChr;
         i += (i_numDelimChr-1); // The for loop will increment by one, so only need to update i by i_numDelimChr minus 1.
      }
      else if(validCharTable[c_curChar])
      {
         pc_retVal[i_numValidChars] = c_curChar;
         ++i_numValidChars;
      }
   }

   pc_retVal[i_numValidChars] = '\0';

   std::string retVal = pc_retVal;
   delete [] pc_retVal;

   return retVal;
}


bool autoDetectHex(const std::string& t_dataToParse)
{
   const char* pc_data = t_dataToParse.c_str();
   int i_numChr = t_dataToParse.length();
   for(int i = 0; i < i_numChr; ++i)
   {
      if( (pc_data[i] >= 'a' && pc_data[i] <= 'f') || (pc_data[i] >= 'A' && pc_data[i] <= 'F') )
      {
         if(pc_data[i] != 'e' && pc_data[i] != 'E') // e can be used for scientic notation.
         {
            return true;
         }
      }
   }
   return false;
}
