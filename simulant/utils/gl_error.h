/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIMULANT_GL_ERROR_H
#define SIMULANT_GL_ERROR_H

#include <string>
#include "../deps/kazlog/kazlog.h"
#include "gl_thread_check.h"

void check_and_log_error(const std::string& function_name);

namespace GLChecker {

/*
 * glGetError is ridiculously slow, so what we do is call it once per frame,
 * if that call returns an error, then we enable it for every call and die
 * when we get an error on the next frame. Make sense?
 */

extern bool USE_GL_GET_ERROR;

void end_of_frame_check();

template<typename Res, typename Func, typename... Args>
struct Checker {
    static Res run(const std::string& function_name, Func&& func, Args&&... args) {
        Res result = func(std::forward<Args>(args)...);
        if(USE_GL_GET_ERROR) {
            check_and_log_error(function_name);
        }
        return result;
    }
};

template<typename Func, typename... Args>
struct Checker<void, Func, Args...> {
    static void run(const std::string& function_name, Func&& func, Args&&... args) {
        func(std::forward<Args>(args)...);
        if(USE_GL_GET_ERROR) {
            check_and_log_error(function_name);
        }
    }
};

template<typename Func>
struct Checker<void, Func> {
    static void run(const std::string& function_name, Func&& func) {
        func();
        if(USE_GL_GET_ERROR) {
            check_and_log_error(function_name);
        }
    }
};

}

template<typename Res=void, typename Func, typename... Args>
Res _GLCheck(const std::string& function_name, Func&& func, Args&&... args) {
    GLThreadCheck::check();
    return GLChecker::Checker<Res, Func, Args...>::run(function_name, std::forward<Func>(func), std::forward<Args>(args)...);
}

#ifndef GLCheck
#define GLCheck(...) _GLCheck(__func__, __VA_ARGS__)
#endif

#endif
