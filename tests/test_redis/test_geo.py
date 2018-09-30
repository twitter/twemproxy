#!/usr/bin/env python
#coding: utf-8

from common import *

def geoadd():
    r = getconn()
    rst = r.geoadd("Sicily", 13.361389, 38.115556, "Palermo",
            15.087269, 37.502669, "Catania")
    assert(rst == 2)

    return r

def test_geodist():
    r = geoadd()
    assert(r.geodist("Sicily", "Palermo",  "Catania") == 166274.1516)
    assert(r.geodist("Sicily", "Palermo", "Catania", "km") == 166.2742)
    assert(r.geodist("Sicily", "Palermo", "Catania", "mi") == 103.3182)
    assert(r.geodist("Sicily", "Foo", "Bar") == None)
    assert(r.geodist("Shenzhen", "Nanshan", "Futian") == None)

def test_geohash():
    r = geoadd()
    rst = r.geohash("Sicily", "Palermo", "Catania")
    assert(rst == ["sqc8b49rny0", "sqdtr74hyu0"])

def test_geopos():
    r = geoadd()
    rst = r.geopos("Sicily", "Palermo", "Catania", "NonExisting")
    assert(rst == [(13.36138933897018433, 38.11555639549629859), 
                   (15.08726745843887329, 37.50266842333162032),
                   None])
    rst = r.geopos("Shenzhen", "Nanshan", "Futian", "Bao'an")
    assert(rst == [None, None, None])

def test_georadius():
    r = geoadd()
    rst = r.georadius("Sicily",15, 37, 200, "km", withdist=True)
    assert(rst == [["Palermo", 190.4424], ["Catania", 56.4413]])

    rst = r.georadius("Sicily", 15, 37, 200, "km", withcoord=True)
    assert(rst == [["Palermo", (13.36138933897018433, 38.11555639549629859)],
                   ["Catania", (15.08726745843887329, 37.50266842333162032)]])

    rst = r.georadius("Sicily", 15, 37, 200, "km", withhash=True)
    assert(rst == [["Palermo", 3479099956230698], ["Catania", 3479447370796909]])

    rst = r.georadius("Sicily", 15, 37, 200, "km",
            withdist=True, withcoord=True, withhash=True)
    assert(rst == [["Palermo", 190.4424, 3479099956230698,
                    (13.36138933897018433, 38.11555639549629859)],
                   ["Catania", 56.4413, 3479447370796909,
                    (15.08726745843887329, 37.50266842333162032)]])

    rst = r.georadius("Shenzhen", 15, 37, 200, "km",
            withdist=True, withcoord=True, withhash=True)
    assert(rst == [])

def test_georadiusbymember():
    r = geoadd()
    assert(r.geoadd("Sicily", 13.583333, 37.316667, "Agrigento") == 1)
    rst = r.georadiusbymember("Sicily", "Agrigento", 100, "km")
    assert(rst == ["Agrigento", "Palermo"])
    rst = r.georadiusbymember("Shenzhen", "Agrigento", 100, "km")
    assert(rst == [])
    assert_fail("could not decode requested zset member",
                r.georadiusbymember, "Sicily", "Nanshan", 100, "km")
