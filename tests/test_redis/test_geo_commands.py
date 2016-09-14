#!/usr/bin/env python
#coding: utf-8

from common import *

def test_geoadd():
    r = getconn()
    
    assert(r.geoadd('Sicily', 13.361389, 38.115556, 'Palermo', 15.087269,  37.502669 , 'Catania')  == 2)