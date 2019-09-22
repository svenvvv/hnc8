/*
 * hnc8
 * Copyright (C) 2019 hundinui
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOG_H
#define LOG_H

#ifdef DEBUG
#   define LOG_DEBUG(...) fprintf(stdout, "DBG [%s]: ", __func__); fprintf(stdout, __VA_ARGS__); fflush(stdout)
#   define LOG_ERROR(...) fprintf(stderr, "ERR [%s]: ", __func__); fprintf(stderr, __VA_ARGS__); fflush(stderr)
#   define LOG(...)       fprintf(stdout, "INFO[%s]: ", __func__); fprintf(stdout, __VA_ARGS__); fflush(stdout)
#else
#   define LOG_DEBUG(...)
#   define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#   define LOG(...)       fprintf(stdout, __VA_ARGS__)
#endif

#endif // LOG_H
