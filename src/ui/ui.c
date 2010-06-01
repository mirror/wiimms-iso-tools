
#include <ctype.h>
#include <getopt.h>

#include "lib-std.h"
#include "version.h"
#include "ui.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			register options		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError RegisterOption
	( const InfoUI_t * iu, int option, int level, bool is_env )
{
    ASSERT(iu);
    ASSERT(iu->opt_info);
    ASSERT(iu->opt_used);
    ASSERT(iu->opt_index);
    ASSERT( iu->n_opt_specific < sizeof(option_t) * CHAR_BIT );

    if ( level > 0 && option >= 0 && option < OPT_INDEX_SIZE )
    {
	int opt_index = iu->opt_index[option];
	if ( opt_index > 0 && opt_index < iu->n_opt_total )
	{
	    TRACE("OPT: RegisterOption(,%02x,%d,%d) option=%02x,%s\n",
			option, level, is_env, opt_index,
			iu->opt_info[opt_index].long_name );
	    u8 * obj = iu->opt_used + opt_index;
	    u32 count = *obj;
	    if (is_env)
	    {
		if ( count < 0x7f )
		{
		    count += level;
		    *obj = count < 0x7f ? count : 0x7f;
		}
	    }
	    else
	    {
		if ( count < 0x80 )
		    count = 0x80;
		count += level;
		*obj = count < 0xff ? count : 0xff;
	    }

	    if ( opt_index < iu->n_opt_specific )
	    {
		if (is_env)
		    env_options |= (option_t)1 << opt_index;
		else
		    used_options |= (option_t)1 << opt_index;
	    }
	    return ERR_OK;
	}
    }
    TRACE("OPT: RegisterOption(,%02x,%d,%d) => WARNING\n",option,level,is_env);
    return ERR_WARNING;
}

///////////////////////////////////////////////////////////////////////////////

enumError VerifySpecificOptions ( const InfoUI_t * iu, const CommandTab_t * cmd )
{
    ASSERT(iu);
    ASSERT(cmd);
    ASSERT( iu->n_opt_specific < sizeof(option_t) * CHAR_BIT );

    option_t forbidden = used_options & ~cmd->opt;

    TRACE("COMMAND: %s\n",cmd->name1);
    TRACE("  - environ options:   %16llx\n",env_options);
    TRACE("  - used options:      %16llx\n",used_options);
    TRACE("  - allowed options:   %16llx\n",cmd->opt);
    TRACE("  - forbidden options: %16llx\n",forbidden);

    if ( forbidden )
    {
	int i;
	for ( i=0; forbidden; i++, forbidden >>= 1 )
	    if ( forbidden & 1 )
	    {
		const InfoOption_t * io = iu->opt_info + i;
		if (io->short_name)
		    ERROR0(ERR_SEMANTIC,"Command '%s' don't allow the option --%s (-%c).\n",
				cmd->name1, io->long_name, io->short_name );
		else
		    ERROR0(ERR_SEMANTIC,"Command '%s' don't allow the option --%s.\n",
				cmd->name1, io->long_name );
	    }
	return ERR_SEMANTIC;
    }

    used_options |= env_options & cmd->opt;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

int GetOptionCount ( const InfoUI_t * iu, int option )
{
    ASSERT(iu);
    ASSERT(iu->opt_info);
    ASSERT(iu->opt_used);
    ASSERT( iu->n_opt_specific < sizeof(option_t) * CHAR_BIT );

    if ( option > 0 && option <  iu->n_opt_total )
    {
	TRACE("GetOptionCount(,%02x) option=%s => %d\n",
		option, iu->opt_info[option].long_name,
		iu->opt_used[option] & OPT_USED_MASK );
	return iu->opt_used[option] & OPT_USED_MASK;
    }

    TRACE("GetOptionCount(,%02x) => INVALID [-1]\n");
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

void DumpUsedOptions ( const InfoUI_t * iu, FILE * f, int indent )
{
    ASSERT(iu);
    ASSERT(iu->opt_info);
    ASSERT(iu->opt_used);
    ASSERT( iu->n_opt_specific < sizeof(option_t) * CHAR_BIT );

    TRACE("OptionUsed[]:\n");
    TRACE_HEXDUMP16(9,0,iu->opt_used,iu->n_opt_total+1);
    TRACE("env_options  = %16llx\n",env_options);
    TRACE("used_options = %16llx\n",used_options);

    if (!f)
	return;

    indent = NormalizeIndent(indent);

    int i;
    for ( i = 0; i < iu->n_opt_total; i++ )
    {
	const u8 opt = iu->opt_used[i];
	if (opt)
	    fprintf(f,"%*s%s %s %2u* [%02x,%s]\n",
			indent, "",
			i <= iu->n_opt_specific ? "CMD" : "GLB",
			opt & 0x80 ? "PRM" : "ENV",
			opt & OPT_USED_MASK,
			i, iu->opt_info[i].long_name );
    }
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 text helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static void PutLines
	( FILE * f, int indent, int fw, int first_line, ccp text )
{
    TRACE("PutLines(,%d,%d,%d,%.20s)\n",indent,fw,first_line,text);

    DASSERT(f);
    ASSERT( indent >= 0 );

    int indent1, fw1;
    if (  indent > first_line )
    {
	indent1 = indent - first_line;
	fw1 = fw - indent;
    }
    else
    {
	indent1 = 0;
	fw1 = fw - first_line;
    }

    fw -= indent;
    if ( fw < 20 )
	fw = 20;

    if ( fw1 < 20 )
    {
	fputc('\n',f);
	indent1 = indent;
	fw1 = fw;
    }
    
    while ( *text )
    {
	// skip blank and control
	while ( *text > 0 && *text <= ' ' )
	    text++;

	// setup
	ccp start = text, last_blank = text;
	ccp max = text + fw1;

	while ( text < max && *text && *text != '\n' )
	{
	    if ( *text > 0 && *text <= ' ' )
		last_blank = text;
	    text++;
	}

	// set back to last blank
	if ( last_blank > start && (u8)*text > ' ' )
	    text = last_blank;

	// print out
	fprintf(f,"%*s%.*s\n", indent1, "", (int)(text-start), start );

	// use standard values for next lines
	indent1 = indent;
	fw1 = fw;
    }
}

///////////////////////////////////////////////////////////////////////////////

static void PrintLines
	( FILE * f, int indent, int fw, int first_line, ccp format, ... )
{
    DASSERT(f);
    DASSERT(format);

    char msg[5000];

    va_list arg;
    va_start(arg,format);
    vsnprintf(msg,sizeof(msg),format,arg);
    va_end(arg);

    PutLines(f,indent,fw,first_line,msg);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   print help			///////////////
///////////////////////////////////////////////////////////////////////////////

void PrintHelp ( const InfoUI_t * iu, FILE * f, int indent )
{
    int cmd = 0;
    if (iu->n_cmd)
    {
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	    if ( param->arg && *param->arg )
	    {
		int cmd_stat;
		const CommandTab_t * ct = ScanCommand(&cmd_stat,param->arg,iu->cmd_tab);
		if (ct)
		    cmd = ct->id;
	    }
    }
    
    PrintHelpCmd(iu,f,indent,cmd);
}

///////////////////////////////////////////////////////////////////////////////

void PrintHelpCmd ( const InfoUI_t * iu, FILE * f, int indent, int cmd )
{
    ASSERT(iu);
    if (!f)
	return;
    indent = NormalizeIndent(indent);

    if ( cmd < 0 || cmd >= iu->n_cmd )
	cmd = 0;
    const InfoCommand_t * ic = iu->cmd_info + cmd;

    const int fw = GetTermWidth(81,40) - 1;

    if (!cmd)
	snprintf(iobuf,sizeof(iobuf),"%s",iu->tool_name );
    else if (ic->name2)
	snprintf(iobuf,sizeof(iobuf),"%s %s|%s",iu->tool_name,ic->name1,ic->name2);
    else
	snprintf(iobuf,sizeof(iobuf),"%s %s",iu->tool_name,ic->name1);

    int len = fprintf(f,"\n%*s%s : ",indent,"",iobuf) - 1;
    PutLines(f,indent+len,fw,len,ic->help);

    len = fprintf(f,"\n%*sSyntax: ", indent, "" ) - 1;
    PutLines(f,indent+len,fw,len,ic->param);
    fputc('\n',f);

    if ( !cmd && iu->n_cmd )
    {
	fprintf(f,"%*sCommands:\n\n",indent,"");

	int fw1 = 0, fw2 = 0;
	const InfoCommand_t * ic;

	for ( ic = iu->cmd_info; ic->name1; ic++ )
	    if (!ic->hidden)
	    {

		const int len = strlen(ic->name1);
		if ( fw1 < len )
		     fw1 = len;
		if ( ic->name2 )
		{
		    const int len = strlen(ic->name2);
		    if ( fw2 < len )
			 fw2 = len;
		}
	    }
	const int fw12 = fw1 + ( fw2 ? fw2 + 3 : 0 );

	for ( ic = iu->cmd_info+1; ic->name1; ic++ )
	    if (!ic->hidden)
	    {
		if (ic->separator)
		    fputc('\n',f);
		int len;
		if ( ic->name2 )
		    len = fprintf(f,"%*s  %-*s | %-*s : ",
			    indent, "", fw1, ic->name1, fw2, ic->name2 );
		else
		    len = fprintf(f,"%*s  %-*s : ",
			    indent, "", fw12, ic->name1 );
		PutLines(f,indent+len,fw,len,ic->help);
	    }
	
	fprintf(f,
		"\n%*sType '%s HELP command' to get command specific help.\n\n",
		indent, "", iu->tool_name );
    }
    
    if ( ic->n_opt )
    {
	fprintf(f,"%*s%sptions:\n\n",
		indent,"", !cmd && iu->n_cmd ? "Global o" : "O" );

	int opt_fw = 0;
	const InfoOption_t ** iop;

	for ( iop = ic->opt; *iop; iop++ )
	{
	    const InfoOption_t * io = *iop;
	    if (io->long_name)
	    {
		int len = strlen(io->long_name);
		if (io->param)
		    len += 1 +  strlen(io->param);
		if ( opt_fw < len )
		     opt_fw = len;
	    }
	}
	if ( opt_fw > 12 )
	    opt_fw = 12;
	opt_fw += indent + 9;

	for ( iop = ic->opt; *iop; iop++ )
	{
	    const InfoOption_t * io = *iop;
	    if ( !io->short_name && !io->long_name )
	    {
		fputc('\n',f);
		continue;
	    }

	    int len;
	    if (!io->short_name)
		len = fprintf(f,"%*s     --%s %s ",
			indent, "",
			io->long_name,
			io->param ? io->param : "" );
	    else if (!io->long_name)
		len = fprintf(f,"%*s  -%c %s ",
			indent, "",
			io->short_name,
			io->param ? io->param : "" );
	    else
		len = fprintf(f,"%*s  -%c --%s %s ",
			indent, "",
			io->short_name,
			io->long_name,
			io->param ? io->param : "" );

	    if ( len > opt_fw )
	    {
		fputc('\n',f);
		PutLines(f,opt_fw,fw,0,io->help);
	    }
	    else
		PutLines(f,opt_fw,fw,len,io->help);
	}
	fputc('\n',f);
    }

    if (!cmd)
	fprintf(f,"%*sMore help is available from %s%s\n\n",
		indent, "", URI_HOME, iu->tool_name );
    else if (!ic->hidden)
    {
	char *dest = iobuf;
	ccp src;
	for ( src = ic->name1; *src; src++ )
	    *dest++ = *src == '_' ? '-' : tolower((int)*src);
	*dest = 0;

	fprintf(f,"%*sMore help is available from %s%s/%s\n\n",
		indent, "", URI_HOME, iu->tool_name, iobuf );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

