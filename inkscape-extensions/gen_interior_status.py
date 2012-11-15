#!/usr/bin/env python 
'''
Copyright (C) 2010 Said LANKRI

##QSvgStyle status generation from existing normal interior

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

class QS_geninteriorstatus(inkex.Effect):
    def __init__(self):
        inkex.Effect.__init__(self)

        self.OptionParser.add_option("--basename",
                        action="store", type="string", 
                        dest="basename", default="button",
                        help="Basename for existing normal frame")

        self.OptionParser.add_option("--focused",
                        action="store", type="inkbool", 
                        dest="focused", default=1,
                        help="Check to generate <basename>-focused elements")

        self.OptionParser.add_option("--pressed",
                        action="store", type="inkbool", 
                        dest="pressed", default=1,
                        help="Check to generate <basename>-pressed elements")

        self.OptionParser.add_option("--toggled",
                        action="store", type="inkbool", 
                        dest="toggled", default=1,
                        help="Check to generate <basename>-toggled elements")

        self.OptionParser.add_option("--disabled",
                        action="store", type="inkbool", 
                        dest="disabled", default=1,
                        help="Check to generate <basename>-disabled elements")

    def copy_interior(self, basename,status, newbasename,newstatus, dx):
        current = self.getElementById(basename+'-'+status)
        if current != None:
            new = copy.deepcopy(current)
            new.set('id', newbasename+'-'+newstatus)
            if new.attrib.has_key('transform'):
                mat = simpletransform.parseTransform(new.get('transform'))
                mat[0][2] = mat[0][2]+dx
            else:
                mat = [[1,0,dx],[0,1,0]]
            new.set('transform', simpletransform.formatTransform(mat))
            current.getparent().append(new)

    def effect(self):

        dx = 30
        if self.options.focused:
            self.copy_interior(self.options.basename,'normal', self.options.basename,'focused', dx)

        if self.options.pressed:
            self.copy_interior(self.options.basename,'normal', self.options.basename,'pressed', 2*dx)

        if self.options.toggled:
            self.copy_interior(self.options.basename,'normal', self.options.basename,'toggled', 3*dx)

        if self.options.pressed:
            self.copy_interior(self.options.basename,'normal', self.options.basename,'disabled', 4*dx)


if __name__ == '__main__':
    e = QS_geninteriorstatus()
    e.affect()


# vim: expandtab shiftwidth=4 tabstop=8 softtabstop=4 encoding=utf-8 textwidth=99
