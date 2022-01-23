// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
#ifndef _STATUS_H
#define _STATUS_H

struct TeensyStatus
{
    int draws;
    int headers;
    int commands;
};

extern TeensyStatus gTeensyStatus;

#endif
