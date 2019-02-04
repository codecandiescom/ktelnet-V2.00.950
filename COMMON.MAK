#
#	Common directives etc for all makefiles
#
clean:
	-del objs\*.res
	-del objs\*.obj
	-del objs\*.csm
	-del objs\*.?0?
	-del bccw32.cfg
	-del *.lib
	-del *.dll
	-del *.exe
	-del *.map
	-del *.m?t
	-del *.bak
	-del *.~*
	-del *.il*
	-del *.tds
