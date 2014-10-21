/* Copyright 2014 Dan Williams. All Rights Reserved.
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
#ifndef D_STRING
#define D_STRING

#include <string>
#include <string.h>
#include <sstream>
#include <vector>
#include <list>

namespace dString
{
   int Len(const std::string& t_inputString);
   std::string Left(const std::string& t_inputString, int i_len);
   std::string Right(const std::string& t_inputString, int i_len);
   std::string Mid(const std::string& t_inputString, int i_startPos);
   std::string Mid(const std::string& t_inputString, int i_startPos, int i_len);
   int InStr(const std::string& t_inputString, const std::string& t_searchString);
   int InStrBack(const std::string& t_inputString, const std::string& t_searchString);
   std::string SplitBackRight(const std::string& t_inputString, const std::string& t_searchString);
   std::string SplitBackLeft(const std::string& t_inputString, const std::string& t_searchString);
   std::string SplitRight(const std::string& t_inputString, const std::string& t_searchString);
   std::string SplitLeft(const std::string& t_inputString, const std::string& t_searchString);
   std::string Replace(const std::string& t_inputString, const std::string& t_searchString, const std::string& t_replaceString);
   std::string Split(std::string* t_inputString, const std::string& t_searchString);
   std::string GetMiddle(std::string* t_inputString, const std::string& t_searchStringStart, const std::string& t_searchStringEnd);
   bool IsStrNum(const std::string& t_inputString);
   std::string GetNumFromStr(const std::string& inStr);
   std::string SplitNumFromStr(std::string& inOutStr);
   std::string Chr(const unsigned char c_chr);
   unsigned char Asc(const std::string& t_inputString);
   std::string Upper(const std::string& t_inputString);
   std::string Lower(const std::string& t_inputString);
   bool Compare(std::string t_cmp1, std::string t_cmp2);
   bool Compare(const char* t_cmp1, std::string t_cmp2);
   bool Compare(std::string t_cmp1, const char* t_cmp2);
   bool Compare(const char* t_cmp1, const char* t_cmp2);
   std::string DontStartWithThis(const std::string& t_inputString, const std::string& t_searchString);
   std::string DontEndWithThis(const std::string& t_inputString, const std::string& t_searchString);
   int Count(const std::string& t_inputString, const std::string& t_searchString);
   std::wstring StringToWString(const std::string& t_inputString);
   std::string WStringToString(const std::wstring& t_inputString);
   
   std::string GetLineEnding();
   std::string ConvertLineEndingToDos(const std::string& t_text);
   std::string ConvertLineEndingToUnix(const std::string& t_text);
   std::string ConvertLineEndingToOS(const std::string& t_text);

   std::string Slice(const std::string& t_inputString, int sliceRHS = 0, int sliceLHS = 0);

   std::vector<std::string> SplitV(const std::string& t_input, const std::string& t_delimiter);
   std::string JoinV(const std::vector<std::string>& t_input, const std::string& t_delimiter);
   
   std::list<std::string> SplitL(const std::string& t_input, const std::string& t_delimiter);
   std::string JoinL(const std::list<std::string>& t_input, const std::string& t_delimiter);
   

   template <class type> bool strTo(const std::string& t_input, type& toVal)
   {
      std::istringstream iss(t_input);
      type tryStrTo;

      // Try to convert string to type.
      iss >> std::noskipws >> tryStrTo; // noskipws means leading whitespace is invalid

      // Make sure the conversion used the entire string without failure.
      bool success = iss.eof() && !iss.fail();

      if(success)
      {
         // Write valid value 
         toVal = tryStrTo;
      }

      // Indicate whether conversion was successful.
      return success; 
   }


}
#endif
