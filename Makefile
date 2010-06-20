#----------------------------------------------------------------
# make manual: http://www.gnu.org/software/make/manual/make.html
#----------------------------------------------------------------

#-------------------------------------------------------------------------------
# global settings

SHELL			= /bin/bash

AUTHOR			= Dirk Clemens
TOOLSET_SHORT		= WIT
TOOLSET_LONG		= Wiimms ISO Tools
WIT_SHORT		= wit
WIT_LONG		= Wiimms ISO Tool
WWT_SHORT		= wwt
WWT_LONG		= Wiimms WBFS Tool

VERSION			= 1.01a

URI_HOME		= http://wit.wiimm.de/
URI_DOWNLOAD		= http://wit.wiimm.de/download
URI_FILE		= http://wit.wiimm.de/file
URI_REPOS		= http://opensvn.wiimm.de/wii/trunk/wiimms-iso-tools/
URI_VIEWVC		= http://wit.wiimm.de/viewvc

URI_WDF			= http://wiimm.de/r/wdf
URI_CISO		= http://wiimm.de/r/ciso
URI_WIIJMANAGER		= http://wiimm.de/r/wiijman
URI_QTWITGUI		= http://wiimm.de/r/qtwitgui
URI_GBATEMP		= http://gbatemp.net/index.php?showtopic=182236\#entry2286365
URI_DOWNLOAD_I386	= $(URI_DOWNLOAD)/$(DISTRIB_I386)
URI_DOWNLOAD_X86_64	= $(URI_DOWNLOAD)/$(DISTRIB_X86_64)
URI_DOWNLOAD_MAC	= $(URI_DOWNLOAD)/$(DISTRIB_MAC)
URI_DOWNLOAD_CYGWIN	= $(URI_DOWNLOAD)/$(DISTRIB_CYGWIN)
URI_TITLES		= http://wiitdb.com/titles.txt

DUMMY			:= $(shell $(SHELL) ./setup.sh)
include Makefile.setup

#-------------------------------------------------------------------------------
# compiler settings

PRE		?= 
CC		= $(PRE)gcc
CPP		= $(PRE)g++
STRIP		= $(PRE)strip

#-------------------------------------------------------------------------------
# files

RM_FILES	= *.{o,d,tmp,bak,exe} */*.{tmp,bak} */*/*.{tmp,bak}
RM_FILES2	= *.{iso,ciso,wdf,wbfs} templates.sed

MODE_FILE	= ./_mode.flag
MODE		= $(shell test -s $(MODE_FILE) && cat $(MODE_FILE))
RM_FILES	+= $(MODE_FILE)

# wbfs: files / size in GiB of WBFS partition / number of ISO files to copy
WBFS_FILE	?= a.wbfs
WBFS_FILES	?= $(WBFS_FILE) b.wbfs c.wbfs d.wbfs
WBFS_SIZE	?= 20
WBFS_COUNT	?= 4

#-------------------------------------------------------------------------------
# tools

MAIN_TOOLS	:= wit wwt wdf-cat wdf-dump
TEST_TOOLS	:= wtest gen-ui wdf
ALL_TOOLS	:= $(sort $(MAIN_TOOLS) $(TEST_TOOLS))

WDF_LINKS	:= WdfCat UnWdf WdfCmp WdfDump Ciso CisoCat UnCiso Wbi

RM_FILES	+= $(ALL_TOOLS) $(WDF_LINKS)

DIR_LIST	=

#-------------------------------------------------------------------------------
# source files

UI_FILES	= ui.def
UI_FILES	+= $(patsubst %,ui-%.c,$(MAIN_TOOLS))
UI_FILES	+= $(patsubst %,ui-%.h,$(MAIN_TOOLS))

SETUP_DIR	= ./setup
SETUP_FILES	= version.h install.sh load-titles.sh wit.def
DIR_LIST	+= $(SETUP_DIR)
RM_FILES2	+= $(SETUP_FILES)

#-------------------------------------------------------------------------------
# object files

# objects of tools
MAIN_TOOLS_OBJ	:= $(patsubst %,%.o,$(MAIN_TOOLS))
TEST_TOOLS_OBJ	:= $(patsubst %,%.o,$(TEST_TOOLS))

# other objects
WIT_O		:= debug.o lib-std.o lib-file.o lib-wdf.o lib-ciso.o lib-sf.o \
		   ui.o iso-interface.o wbfs-interface.o patch.o \
		   titles.o match-pattern.o dclib-utf8.o \
		   sha1dgst.o sha1_one.o
LIBWBFS_O	:= file-formats.o libwbfs.o wiidisc.o rijndael.o

# object groups
UI_OBJECTS	:= $(sort $(MAIN_TOOLS_OBJ))
C_OBJECTS	:= $(sort $(TEST_TOOLS_OBJ) $(WIT_O) $(LIBWBFS_O))
ASM_OBJECTS	:= ssl-asm.o

# all objects + sources
ALL_OBJECTS	:= $(sort $(WIT_O) $(LIBWBFS_O) $(ASM_OBJECTS))
ALL_SOURCES	:= $(patsubst %.o,%.c,$(UI_OBJECTS) $(C_OBJECTS) $(ASM_OBJECTS))

#-------------------------------------------------------------------------------

INSTALL_PATH	= /usr/local
INSTALL_SCRIPTS	= install.sh load-titles.sh
RM_FILES	+= $(INSTALL_SCRIPTS)
SCRIPTS		= ./scripts
TEMPLATES	= ./templates
MODULES		= $(TEMPLATES)/module
GEN_TEMPLATE	= ./gen-template.sh
UI		= ./src/ui
DIR_LIST	+= $(SCRIPTS) $(TEMPLATES) $(MODULES) $(UI)

VPATH		+= src src/libwbfs src/crypto $(UI) work
DIR_LIST	+= src src/libwbfs src/crypto $(UI) work

DEFINES1	=  -DLARGE_FILES -D_FILE_OFFSET_BITS=64
DEFINES1	+= -DWIT		# enable WIT specific modifications in libwbfs
DEFINES1	+= -DDEBUG_ASSERT	# enable ASSERTions in release version too
DEFINES1	+= -DEXTENDED_ERRORS=1	# enable extended error messages (function,line,file)
DEFINES		=  $(strip $(DEFINES1) $(MODE) $(XDEF))

CFLAGS		=  -fomit-frame-pointer -fno-strict-aliasing
CFLAGS		+= -Wall -Wno-parentheses -Wno-unused-function
CFLAGS		+= -O3 -Isrc/libwbfs -Isrc -I$(UI) -I. -Iwork
CFLAGS		+= $(XFLAGS)
CFLAGS		:= $(strip $(CFLAGS))

DEPFLAGS	+= -MMD

LDFLAGS		+= -static-libgcc
#LDFLAGS	+= -static
LDFLAGS		:= $(strip $(LDFLAGS))

LIBS		+= $(XLIBS)

DISTRIB_RM	= ./wit-v$(VERSION)-r
DISTRIB_BASE	= wit-v$(VERSION)-r$(REVISION_NEXT)
DISTRIB_PATH	= ./$(DISTRIB_BASE)-$(SYSTEM)
DISTRIB_I386	= $(DISTRIB_BASE)-i386.tar.gz
DISTRIB_X86_64	= $(DISTRIB_BASE)-x86_64.tar.gz
DISTRIB_MAC	= $(DISTRIB_BASE)-mac.tar.gz
DISTRIB_CYGWIN	= $(DISTRIB_BASE)-cygwin.zip
DISTRIB_FILES	= gpl-2.0.txt $(INSTALL_SCRIPTS)

DOC_FILES	= doc/*.txt
TITLE_FILES	= titles.txt $(patsubst %,titles-%.txt,$(LANGUAGES))
LANGUAGES	= de es fr it ja ko nl pt

BIN_FILES	= $(MAIN_TOOLS)
LIB_FILES	= $(TITLE_FILES)

CYGWIN_DIR	= pool/cygwin
CYGWIN_BIN	= cygwin1.dll
CYGWIN_BIN_SRC	= $(patsubst %,$(CYGWIN_DIR)/%,$(CYGWIN_BIN))

DIR_LIST_BIN	= $(SCRIPTS) bin
DIR_LIST	+= $(DIR_LIST_BIN)
DIR_LIST	+= lib doc work pool makefiles-local edit-list

#
###############################################################################
# default rule

default_rule: all
	@echo "HINT: try 'make help'"

# include this behind the default rule
-include $(ALL_SOURCES:.c=.d)

#
###############################################################################
# general rules

$(ALL_TOOLS): %: %.o $(ALL_OBJECTS) Makefile
	@echo "***    tool $@ $(MODE)"
	@$(CC) $(CFLAGS) $(DEFINES) $(LDFLAGS) $@.o $(ALL_OBJECTS) $(LIBS) -o $@
	@if test -f $@.exe; then $(STRIP) $@.exe; else $(STRIP) $@; fi
	@mkdir -p bin/debug
	@cp -p $@ bin
	@if test -s $(MODE_FILE) && grep -Fq -e -DDEBUG $(MODE_FILE); then cp -p $@ bin/debug; fi

#--------------------------

$(UI_OBJECTS): %.o: %.c ui-%.c ui-%.h version.h Makefile
	@echo "*** +object $@ $(MODE)"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

#--------------------------

$(C_OBJECTS): %.o: %.c version.h Makefile
	@echo "***  object $@ $(MODE)"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

#--------------------------

$(ASM_OBJECTS): %.o: %.S Makefile
	@echo "***     asm $@ $(MODE)"
	@$(CC) $(CFLAGS) $(DEPFLAGS) $(DEFINES) -c $< -o $@

#--------------------------

$(SETUP_FILES): templates.sed $(SETUP_DIR)/$@
	@echo "***  create $@"
	@chmod 775 $(GEN_TEMPLATE)
	@$(GEN_TEMPLATE) $@

#--------------------------

$(UI_FILES): gen-ui.c tab-ui.c ui.h | gen-ui
	@echo "***     run gen-ui"
	@./gen-ui

.PHONY : ui
ui : gen-ui
	@echo "***     run gen-ui"
	@./gen-ui

#
###############################################################################
# specific rules in alphabetic order

.PHONY : all
all: $(ALL_TOOLS) $(INSTALL_SCRIPTS)

.PHONY : all+
all+: clean+ all distrib

.PHONY : all++
all++: clean+ all titles distrib

#
#--------------------------

.PHONY : ch+
ch+:	chmod chown chgrp

#
#--------------------------

.PHONY : chmod
chmod:
	@echo "***   chmod 775/664"
	@for d in . $(DIR_LIST); do test -d "$$d" && chmod ug+rw "$$d"/*; done
	@for d in $(DIR_LIST); do test -d "$$d" && chmod 775 "$$d"; done
	@find . -name '*.sh' -exec chmod 775 {} +
	@for t in $(ALL_TOOLS); do test -f "$$t" && chmod 775 "$$t"; done || true

#
#--------------------------

.PHONY : chown
chown:
	@echo "***   chown -R $$( stat -c%u . 2>/dev/null || stat -f%u . ) ."
	@chown -R "$$( stat -c%u . 2>/dev/null || stat -f%u . )" .

#
#--------------------------

.PHONY : chgrp
chgrp:
	@echo "***   chgrp -R $$( stat -c%g . 2>/dev/null || stat -f%g . ) ."
	@chgrp -R "$$( stat -c%g . 2>/dev/null || stat -f%g . )" .

#
#--------------------------

.PHONY : clean
clean:
	@echo "***      rm output files + distrib"
	@rm -f $(RM_FILES)
	@rm -fr $(DISTRIB_RM)*

.PHONY : clean+
clean+: clean
	@echo "***      rm test files + template output"
	@rm -f $(RM_FILES2)
	-@rm -fr doc

.PHONY : clean++
clean++: clean+
	@test -d .svn && svn st | sort -k2 || true

#
#--------------------------

.PHONY : debug
debug:
	@echo "***  enable debug (-DDEBUG)"
	@rm -f *.o $(ALL_TOOLS)
	@echo "-DDEBUG" >>$(MODE_FILE)
	@sort $(MODE_FILE) | uniq > $(MODE_FILE).tmp
# 2 steps because a cygwin mv failure
	@cp $(MODE_FILE).tmp $(MODE_FILE)
	@rm -f $(MODE_FILE).tmp

#
#--------------------------

.PHONY : distrib
distrib: all doc gen-distrib wit.def

.PHONY : gen-distrib
gen-distrib:
	@echo "***  create $(DISTRIB_PATH)"

ifeq ($(SYSTEM),cygwin)

	@rm -rf $(DISTRIB_PATH)/* 2>/dev/null || true
	@rm -rf $(DISTRIB_PATH) 2>/dev/null || true
	@mkdir -p $(DISTRIB_PATH)/bin $(DISTRIB_PATH)/doc
	@cp -p gpl-2.0.txt $(DISTRIB_PATH)
	@cp -p $(MAIN_TOOLS) $(CYGWIN_BIN_SRC) $(DISTRIB_PATH)/bin
	@( cd lib; cp $(TITLE_FILES) ../$(DISTRIB_PATH)/bin )
	@cp -p $(DOC_FILES) $(DISTRIB_PATH)/doc
	@zip -roq $(DISTRIB_PATH).zip $(DISTRIB_PATH)

else
	@rm -rf $(DISTRIB_PATH)
	@mkdir -p $(DISTRIB_PATH)/bin $(DISTRIB_PATH)/scripts $(DISTRIB_PATH)/lib $(DISTRIB_PATH)/doc

	@cp -p $(DISTRIB_FILES) $(DISTRIB_PATH)
	@cp -p $(MAIN_TOOLS) $(DISTRIB_PATH)/bin
	@cp -p lib/*.txt $(DISTRIB_PATH)/lib
	@cp -p $(DOC_FILES) $(DISTRIB_PATH)/doc
	@cp -p $(SCRIPTS)/*.{sh,txt} $(DISTRIB_PATH)/scripts

	@chmod -R 664 $(DISTRIB_PATH)
	@chmod a+x $(DISTRIB_PATH)/*.sh $(DISTRIB_PATH)/scripts/*.sh $(DISTRIB_PATH)/bin*/*
	@chmod -R a+X $(DISTRIB_PATH)
#	-@chown -R --reference=. $(DISTRIB_PATH) >/dev/null

	@tar -czf $(DISTRIB_PATH).tar.gz $(DISTRIB_PATH)
endif

#
#--------------------------

.PHONY : doc
doc: $(MAIN_TOOLS) templates.sed gen-doc

.PHONY : gen-doc
gen-doc:
	@echo "***  create documentation"
	@chmod ug+x $(GEN_TEMPLATE)
	@$(GEN_TEMPLATE)
	@cp -p doc/WDF.txt .

#
#--------------------------

.PHONY : flags
flags:
	@echo  ""
	@echo  "DEFINES: $(DEFINES)"
	@echo  ""
	@echo  "CFLAGS:  $(CFLAGS)"
	@echo  ""
	@echo  "LDFLAGS: $(LDFLAGS)"
	@echo  ""
	@echo  "LIBS: $(LIBS)"
	@echo  ""

#
#--------------------------

.PHONY : install
install: all
	@chmod a+x install.sh
	@./install.sh --make

.PHONY : install+
install+: clean+ all
	@chmod a+x install.sh
	@./install.sh --make

#
#--------------------------

.PHONY : predef
predef:
	@gcc -E -dM none.c | sort

#
#--------------------------

templates.sed: Makefile
	@echo "***  create templates.sed"
	@echo -e '' \
		'/^~/ d;\n' \
		's|@.@@@|$(VERSION)|g;\n' \
		's|@@@@-@@-@@|$(DATE)|g;\n' \
		's|@@:@@:@@|$(TIME)|g;\n' \
		's|@@AUTHOR@@|$(AUTHOR)|g;\n' \
		's|@@TOOLSET-SHORT@@|$(TOOLSET_SHORT)|g;\n' \
		's|@@TOOLSET-LONG@@|$(TOOLSET_LONG)|g;\n' \
		's|@@WIT-SHORT@@|$(WIT_SHORT)|g;\n' \
		's|@@WIT-LONG@@|$(WIT_LONG)|g;\n' \
		's|@@WWT-SHORT@@|$(WWT_SHORT)|g;\n' \
		's|@@WWT-LONG@@|$(WWT_LONG)|g;\n' \
		's|@@VERSION@@|$(VERSION)|g;\n' \
		's|@@REV@@|$(REVISION)|g;\n' \
		's|@@REV-NUM@@|$(REVISION_NUM)|g;\n' \
		's|@@REV-NEXT@@|$(REVISION_NEXT)|g;\n' \
		's|@@BINTIME@@|$(BINTIME)|g;\n' \
		's|@@DATE@@|$(DATE)|g;\n' \
		's|@@TIME@@|$(TIME)|g;\n' \
		's|@@INSTALL-PATH@@|$(INSTALL_PATH)|g;\n' \
		's|@@BIN-FILES@@|$(BIN_FILES)|g;\n' \
		's|@@LIB-FILES@@|$(LIB_FILES)|g;\n' \
		's|@@LANGUAGES@@|$(LANGUAGES)|g;\n' \
		's|@@DISTRIB-I386@@|$(DISTRIB_I386)|g;\n' \
		's|@@DISTRIB-X86_64@@|$(DISTRIB_X86_64)|g;\n' \
		's|@@DISTRIB-MAC@@|$(DISTRIB_MAC)|g;\n' \
		's|@@DISTRIB-CYGWIN@@|$(DISTRIB_CYGWIN)|g;\n' \
		's|@@URI-FILE@@|$(URI_FILE)|g;\n' \
		's|@@URI-REPOS@@|$(URI_REPOS)|g;\n' \
		's|@@URI-VIEWVC@@|$(URI_VIEWVC)|g;\n' \
		's|@@URI-HOME@@|$(URI_HOME)|g;\n' \
		's|@@URI-DOWNLOAD@@|$(URI_DOWNLOAD)|g;\n' \
		's|@@URI-WDF@@|$(URI_WDF)|g;\n' \
		's|@@URI-CISO@@|$(URI_CISO)|g;\n' \
		's|@@URI-WIIJMANAGER@@|$(URI_WIIJMANAGER)|g;\n' \
		's|@@URI-QTWITGUI@@|$(URI_QTWITGUI)|g;\n' \
		's|@@URI-GBATEMP@@|$(URI_GBATEMP)|g;\n' \
		's|@@URI-DOWNLOAD-I386@@|$(URI_DOWNLOAD_I386)|g;\n' \
		's|@@URI-DOWNLOAD-X86_64@@|$(URI_DOWNLOAD_X86_64)|g;\n' \
		's|@@URI-DOWNLOAD-MAC@@|$(URI_DOWNLOAD_MAC)|g;\n' \
		's|@@URI-DOWNLOAD-CYGWIN@@|$(URI_DOWNLOAD_CYGWIN)|g;\n' \
		's|@@URI-TITLES@@|$(URI_TITLES)|g;\n' \
		>templates.sed

#
#--------------------------

.PHONY : test
test:
	@echo "***  enable test (-DTEST)"
	@rm -f *.o $(ALL_TOOLS)
	@echo "-DTEST" >>$(MODE_FILE)
	@sort $(MODE_FILE) | uniq > $(MODE_FILE).tmp
# 2 steps because a cygwin mv failure
	@cp $(MODE_FILE).tmp $(MODE_FILE)
	@rm -f $(MODE_FILE).tmp

#
#--------------------------

.PHONY : testtrace
testtrace:
	@echo "***  enable testtrace (-DTESTTRACE)"
	@rm -f *.o $(ALL_TOOLS)
	@echo "-DTESTTRACE" >>$(MODE_FILE)
	@sort $(MODE_FILE) | uniq > $(MODE_FILE).tmp
# 2 steps because a cygwin mv failure
	@cp $(MODE_FILE).tmp $(MODE_FILE)
	@rm -f $(MODE_FILE).tmp

#
#--------------------------

.PHONY : titles
titles: wit load-titles.sh gen-titles

.PHONY : gen-titles
gen-titles:
	@chmod a+x load-titles.sh
	@./load-titles.sh --make

#
#--------------------------

.PHONY : tools
tools: $(ALL_TOOLS)

#
#--------------------------

%.wbfs: wwt
	@echo "***  create $@, $(WBFS_SIZE)G, add smallest $(WBFS_COUNT) ISOs"
	@rm -f $@
	@./wwt format --force --inode --size $(WBFS_SIZE)- "$@"
	@stat --format="%b|%n" pool/iso/*.iso \
		| sort -n \
		| awk '-F|' '{print $$2}' \
		| head -n$(WBFS_COUNT) \
		| ./wwt -A -p "$@" add @- -v

#
#--------------------------

.PHONY : format-wbfs
format-wbfs:
	@echo "***  create $(WBFS_FILES), size=$(WBFS_SIZE)G"
	@rm -f $(WBFS_FILES)
	@s=512; \
	    for w in $(WBFS_FILES); \
		do ./wwt format -qfs $(WBFS_SIZE)- --sector-size=$$s --inode "$$w"; \
		let s*=2; \
	    done

#
#--------------------------

.PHONY : wbfs
wbfs: wwt format-wbfs gen-wbfs

.PHONY : gen-wbfs
gen-wbfs: format-wbfs
	@echo "***  charge $(WBFS_FILE)"
	@stat --format="%b|%n" pool/wdf/*.wdf \
		| sort -n \
		| awk '-F|' '{print $$2}' \
		| head -n$(WBFS_COUNT) \
		| ./wwt -A -p @<(ls $(WBFS_FILE)) add @-
	@echo
	@./wwt f -l $(WBFS_FILES)
	@./wwt ll $(WBFS_FILE) --mtime

#
#--------------------------

.PHONY : wbfs+
wbfs+: wwt format-wbfs gen-wbfs+

.PHONY : gen-wbfs+
gen-wbfs+: format-wbfs
	@echo "***  charge $(WBFS_FILES)"
	@stat --format="%b|%n" pool/iso/*.iso \
		| sort -n \
		| awk '-F|' '{print $$2}' \
		| head -n$(WBFS_COUNT) \
		| ./wwt -A -p @<(ls $(WBFS_FILES)) add @-
	@echo
	@./wwt f -l $(WBFS_FILES)
	@./wwt ll $(WBFS_FILES)
	@echo "WBFS: this is not a wbfs file" >no.wbfs

#
#--------------------------

.PHONY : xwbfs
xwbfs: wwt
	@echo "***  create x.wbfs, 1TB, sec-size=2048 and then with sec-size=512"
	@./wwt init -qfs1t x.wbfs --inode --sector-size=2048
	@sleep 2
	@./wwt init -qfs1t x.wbfs --inode --sector-size=512

#
#--------------------------

.PHONY : wdf-links
wdf-links:
	@echo "***    link $(WDF_LINKS) -> wdf"
	@for l in $(WDF_LINKS); do rm -f $$l; ln -s wdf $$l; done

#
###############################################################################
# help rule

.PHONY : help
help:
	@echo  ""
	@echo  "$(DATE) $(TIME) - $(VERSION) - svn r$(REVISION):$(REVISION_NEXT)"
	@echo  ""
	@echo  " make		:= make all"
	@echo  " make all	make all tools and install scripts"
	@echo  " make all+	:= make clean+ all distrib"
	@echo  " make all++	:= make clean+ all titles distrib"
	@echo  " make _tool_	compile only the named '_tool_' (wit,wwt,...)"
	@echo  " make tools	make all tools"
	@echo  ""
	@echo  " make clean	remove all output files"
	@echo  " make clean+	make clean & rm test_files & rm template_output"
	@echo  ""
	@echo  " make debug	enable '-DDEBUG'"
	@echo  " make test	enable '-DTEST'"
	@echo  " make testtrace	enable '-DTESTTRACE'"
	@echo  " make flags	print DEFINES, CFLAGS and LDFLAGS"
	@echo  ""
	@echo  " make doc	generate doc files from their templates"
	@echo  " make distrib	make all & build $(DISTRIB_FILE)"
	@echo  " make titles	get titles from $(URI_TITLES)"
	@echo  " make install	make all & copy tools to $(INSTALL_PATH)"
	@echo  " make install+	make clean+ all & copy tools to $(INSTALL_PATH)"
	@echo  ""
	@echo  " make chmod	change mode 775/644 for known dirs and files"
	@echo  " make chown	change owner of all dirs+files to owner of ."
	@echo  " make chgrp	change group of all dirs+files to group of ."
	@echo  " make ch+	:= make chmod chown chgrp"
	@echo  ""
	@echo  " make %.wbfs	gen %.wbfs, $(WBFS_SIZE)G, add smallest $(WBFS_COUNT) ISOs"
	@echo  " make wbfs	gen $(WBFS_FILE), $(WBFS_SIZE)G, add smallest $(WBFS_COUNT) ISOs"
	@echo  " make wbfs+	gen $(WBFS_FILES), $(WBFS_SIZE)G, add smallest $(WBFS_COUNT) ISOs"
	@echo  ""
	@echo  " make help	print this help"
	@echo  ""

#
###############################################################################
# local definitions

-include makefiles-local/Makefile.local.$(SYSTEM)
-include Makefile.user

