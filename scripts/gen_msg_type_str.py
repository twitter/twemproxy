#!/usr/bin/env python
#coding: utf-8
#file   : gen_msg_type_str.py
#author : ning
#date   : 2014-05-28 13:45:16


import os
import re

content = file('src/nc_message.h').read()

m = re.search(r'typedef enum msg_type {(.*?)}', content, re.DOTALL)
body = m.group(1)
print body

print '''
inline char *
nc_msg_type_str(msg_type_t type)
{
    switch (type) {
'''

for line in body.split():
    if line.find(',') > -1:
        msg_type = line.split(',')[0].strip()
        msg_type_str = msg_type.split('_')[-1]
        print '    case %s: ' % msg_type
        print '        return "%s"; ' % msg_type_str

print '''
    default:
+        return "UNKNOWN";
    }
}
'''

