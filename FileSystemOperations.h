/* Copyright 2013 - 2014 Dan Williams. All Rights Reserved.
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
#ifndef FileSystemOperations_h
#define FileSystemOperations_h

#include <string>
#include <list>

namespace fso
{
   typedef struct
   {
      std::string t_path;
      bool b_isDir;
   }tDirListing;
   typedef std::list<tDirListing> tDirContents;
   
   std::string dirSep();
   std::string dirSepToOS(const std::string& t_path);

   void GetDirContents(tDirContents& t_dirContents, std::string t_dir, bool b_recursive);
   
   bool DirExists(std::string t_dir);
   bool FileExists(std::string t_path);
   bool createDir(std::string t_dir);
   bool recursiveCreateDir(std::string t_dir);
   std::string GetDir(std::string t_path);
   std::string GetFile(std::string t_path);
   std::string GetExt(std::string t_path);
   std::string RemoveExt(std::string t_path);
   std::string GetFileNameNoExt(std::string t_path);
   std::string ReadFile(std::string t_path);
   void WriteFile(std::string t_path, std::string t_fileText);
   void WriteFile(std::string t_path, char* pc_fileText, int i_outSizeBytes);
   void AppendFile(std::string t_path, std::string t_fileText);
   
   bool ComparePath(tDirListing t_comp1, tDirListing t_comp2);

   std::string DontEndWithDirSep(const std::string& t_path);
   std::string EndWithDirSep(const std::string& t_path);

}

#endif
