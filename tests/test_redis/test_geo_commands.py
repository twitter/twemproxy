#!/usr/bin/env python
#coding: utf-8

from common import *

def test_geoadd():
    r = getconn()
    
    assert(r.geoadd('Sicily', 
        13.361389, 38.115556, 'Palermo', 
        15.087269,  37.502669 , 'Catania')  == 2)

def test_geohash():
    r = getconn()
    
    r.geoadd('Sicily', 
        13.361389, 38.115556, 'Palermo', 
        15.087269,  37.502669 , 'Catania')
    
    res = r.geohash('Sicily', 'Palermo' , 'Catania')
    assert(res  == ['sqc8b49rny0', 'sqdtr74hyu0'])
    
def test_geodist():
    r = getconn()
    
    r.geoadd('Sicily', 
        13.361389, 38.115556, 'Palermo', 
        15.087269,  37.502669 , 'Catania')
        
    assert(r.geodist('Sicily', 'Palermo' , 'Catania') == 166274.1516)
    
    assert(r.geodist('Sicily', 'Palermo' , 'Catania', 'km') == 166.2742)
    
    assert(r.geodist('Sicily', 'Palermo' , 'Catania', 'mi') == 103.3182)
    