#!/usr/bin/env python 
'''
Copyright (C) 2010 Said LANKRI

##QSvgStyle copy interior set with basename change

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
'''

import inkex
import simplestyle, sys, copy, simpletransform
from math import *

class QS_copyinterior(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)

        self.OptionParser.add_option("--basename",
                        action="store", type="string", 
                        dest="basename", default="button",
                        help="Basename for existing normal interior")

        self.OptionParser.add_option("--newbasename",
                        action="store", type="string", 
                        dest="newbasename", default="copybutton",
                        help="New basename for copied interior")

        self.OptionParser.add_option("--normal",
                        action="store", type="inkbool", 
                        dest="normal", default=1,
                        help="Copy normal interior")

        self.OptionParser.add_option("--focused",
                        action="store", type="inkbool", 
                        dest="focused", default=1,
                        help="Copy focused interior")

        self.OptionParser.add_option("--pressed",
                        action="store", type="inkbool", 
                        dest="pressed", default=1,
                        help="copy pressed interior")

        self.OptionParser.add_option("--toggled",
                        action="store", type="inkbool", 
                        dest="toggled", default=1,
                        help="copy toggled interior")

        self.OptionParser.add_option("--disabled",
                        action="store", type="inkbool", 
                        dest="disabled", default=1,
                        help="copy disabled interior")

    def copy_interior(self, basename,status, newbasename,newstatus):
        current = self.getElementById(basename+'-'+status)
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus)
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[1][2] = mat[1][2]-30
            else:
                mat = [[1,0,0],[0,1,-30]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

    def effect(self):

        if self.options.normal:
            self.copy_interior(self.options.basename,'normal', self.options.newbasename,'normal')

        if self.options.focused:
            self.copy_interior(self.options.basename,'focused', self.options.newbasename,'focused')

        if self.options.pressed:
            self.copy_interior(self.options.basename,'pressed', self.options.newbasename,'pressed')

        if self.options.toggled:
            self.copy_interior(self.options.basename,'toggled', self.options.newbasename,'toggled')

        if self.options.pressed:
            self.copy_interior(self.options.basename,'disabled', self.options.newbasename,'disabled')


if __name__ == '__main__':
    e = QS_copyinterior()
    e.affect()


# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 encoding=utf-8 textwidth=99
