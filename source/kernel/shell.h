/*
 *
 * Copyright (c) 2016 Project Raphine
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Yuchiki
 * 
 */


/*this shell-like program is substituted later*/

#ifndef __RAPH_KERNEL_SHELL_H__
#define __RAPH_KERNEL_SHELL_H__

class Shell {
 public:
  void Setup();
  void Register(const char *name, void (*func)(int argc, const char *argv[]));
  void ReadCh(char c);

 private:
  void Exec(const char *name, int argc, const char* argv[]);

  static const int kBufSize = 10;
  static const int kNameSize = 10;
  int _next_buf = 0;
  struct NameFuncMapping {

    char name[kNameSize];
    void (*func)(int argc, const char *argv[]);
  } _name_func_mapping[kBufSize];
  class Liner { //convert char inputs to a line input
  public:
    void Setup(Shell *shell) {
      _shell = shell;
      Reset();
    }
    void ReadCh(char c);
  private:
    void Tokenize();
    void Reset();
    static const int kCommandSize = 100;
    static const int kArgumentMax = 10;
    char _command[kCommandSize] = "";
    char *_arguments[kArgumentMax + 1];
    int _next_command = 0;
    int _argc = 0;
    Shell *_shell;
  } _liner;
};
#endif //__RAPH_KERNEL_SHELL_H__
