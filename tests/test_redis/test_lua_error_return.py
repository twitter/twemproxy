#!/usr/bin/env python
#coding: utf-8

from unittest import TestCase

from redis import Redis, ResponseError

from common import *


class LuaReturnErrorTestCase(TestCase):

    def test_lua_return_error(self):
        """Test the error described on issue 404 is fixed.
        
        https://github.com/twitter/twemproxy/issues/404
        
        """
        r = getconn()
        p = r.pipeline(transaction=False)

        p.set("test_key", "bananas!")
        p.eval('return {err="dummyerror"}', 1, "dummy_key")
        p.get("test_key")

        set_result, eval_result, get_result = p.execute(raise_on_error=False)

        self.assertTrue(set_result)
    
        self.assertIsInstance(eval_result, ResponseError)
        self.assertEqual(eval_result.message, "dummyerror")
    
        self.assertEqual(get_result, "bananas!")
