/**
 * file:    serialize.cpp
 * created: 2016-11-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016-2017 - all rights reserved
 */

struct StructMemberInfo;

void serialize_load_conf(
    FilePathView path,
    StructMemberInfo *members,
    i32 num_members,
    void *out);

void serialize_save_conf(
    FilePathView path,
    StructMemberInfo *members,
    i32 num_members,
    void *ptr);