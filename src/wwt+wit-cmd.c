
/***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit http://wit.wiimm.de/ for project details and sources.           *
 *                                                                         *
 *   Copyright (c) 2009-2010 by Dirk Clemens <wiimm@wiimm.de>              *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//   This file is included by wwt.c and wit.c and contains common commands.  //
///////////////////////////////////////////////////////////////////////////////

enumError cmd_error()
{
    if (!n_param)
    {
	if ( print_sections )
	{
	    int i;
	    for ( i=0; i<ERR__N; i++ )
		printf("\n[error-%02u]\ncode=%u\nname=%s\ntext=%s\n",
			i, i, GetErrorName(i), GetErrorText(i));
	}
	else
	{
	    const bool print_header = !OptionUsed[OPT_NO_HEADER];

	    if (print_header)
	    {
		print_title(stdout);
		printf(" List of error codes\n\n");
	    }
	    int i;

	    // calc max_wd
	    int max_wd = 0;
	    for ( i=0; i<ERR__N; i++ )
	    {
		const int len = strlen(GetErrorName(i));
		if ( max_wd < len )
		    max_wd = len;
	    }

	    // print table
	    for ( i=0; i<ERR__N; i++ )
		printf("%3d : %-*s : %s\n",i,max_wd,GetErrorName(i),GetErrorText(i));

	    if (print_header)
		printf("\n");
	}

	return ERR_OK;
    }

    int stat;
    long num = ERR__N;
    if ( n_param != 1 )
	stat = ERR_SYNTAX;
    else
    {
	char * end;
	num = strtoul(first_param->arg,&end,10);
	stat = *end ? ERR_SYNTAX : num < 0 || num >= ERR__N ? ERR_SEMANTIC : ERR_OK;
    }

    if (print_sections)
	printf("\n[error]\ncode=%lu\nname=%s\ntext=%s\n",
		num, GetErrorName(num), GetErrorText(num));
    else if (long_count)
	printf("%s\n",GetErrorText(num));
    else
	printf("%s\n",GetErrorName(num));
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_compr()
{
    const bool print_header = !print_sections
			    && long_count > 1
			    && !OptionUsed[OPT_NO_HEADER];
    if (print_header)
	printf(	"\n"
		" num  name    alternative names\n"
		"--------------------------------\n");

    const bool have_param = n_param > 0;
    if (!n_param)
    {
	int i;
	for ( i = 0; i < WD_COMPR__N; i++ )
	    AddParam(wd_get_compression_name(i,0),false);
    }

    int err_count = 0;
    if (print_sections)
    {
	if (!have_param)
	{
	    printf( "\n[compression-methods]\n"
		    "n-methods=%u\n"
		    "default-method=%u %s\n"
		    "fastest-method=%u %s\n"
		    "best-method=%u %s\n",
		    WD_COMPR__N,
		    WD_COMPR__DEFAULT, wd_get_compression_name(WD_COMPR__DEFAULT,"-"),
		    WD_COMPR__FASTEST, wd_get_compression_name(WD_COMPR__FASTEST,"-"),
		    WD_COMPR__BEST,    wd_get_compression_name(WD_COMPR__BEST,"-") );
	}

	int index = 0;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next, index++ )
	{
	    int level;
	    u32 chunk_size;
	    wd_compression_t compr = ScanCompression(param->arg,true,&level,&chunk_size);
	    if ( compr == (wd_compression_t)-1 )
		err_count++;
	    printf( "\n[compression-method-%u]\n"
		    "num=%d\n"
		    "name=%s"
		    "%s%d\n"
		    "%s%s%s",
		    index, compr, wd_get_compression_name(compr,"-"),
		    level ? "\nlevel=" : "", level,
		    compr == WD_COMPR__DEFAULT ? "is-default=1\n" : "",
		    compr == WD_COMPR__FASTEST ? "is-fastest=1\n" : "",
		    compr == WD_COMPR__BEST    ? "is-best=1\n" : "" );
	 #ifdef NO_BZIP2
	    if ( compr == WD_COMPR_BZIP2 )
		fputs("not-supported=1\n",stdout);
	 #endif
	}
	putchar('\n');
    }
    else if ( long_count > 0 )
    {
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	{
	    int level;
	    u32 chunk_size;
	    wd_compression_t compr = ScanCompression(param->arg,true,&level,&chunk_size);
	    if ( compr == (wd_compression_t)-1 )
	    {
		err_count++;
		printf("   - -\n");
	    }
	    else if ( long_count == 1 )
	    {
	     #ifdef NO_BZIP2
		if ( have_param || compr != WD_COMPR_BZIP2 )
	     #endif
		printf("%s\n",wd_print_compression(0,0,compr,level,chunk_size,3));
	    }
	    else
	    {
		printf(" %-4s %-7s",
			wd_print_compression(0,0,compr,level,0,1),
			wd_print_compression(0,0,compr,level,chunk_size,2) );
		if ( long_count > 1 )
		{
		    if ( compr == WD_COMPR__DEFAULT )
			printf(" DEFAULT");
		    if ( compr == WD_COMPR__FASTEST )
			printf(" FASTEST");
		    if ( compr == WD_COMPR__BEST )
			printf(" BEST");
		 #ifdef NO_BZIP2
		    if ( compr == WD_COMPR_BZIP2 )
			printf("    (not supported)");
		 #endif
		}
		putchar('\n');
	    }
	}
    }
    else
    {
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	{
	    int level;
	    u32 chunk_size;
	    wd_compression_t compr = ScanCompression(param->arg,true,&level,&chunk_size);
	 #ifdef NO_BZIP2
	    if ( !have_param && compr == WD_COMPR_BZIP2 )
		continue; // ignore it
	 #endif
	    if ( compr == (wd_compression_t)-1 )
		err_count++;
	    printf("%s\n",wd_print_compression(0,0,compr,level,chunk_size,2));
	}
    }

    if (print_header)
	putchar('\n');

    return err_count ? ERR_WARNING : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_exclude()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AtFileHelper(param->arg,true,true,AddExcludeID);

    SetupExcludeDB();
    DumpIDDB(&exclude_db,stdout);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_titles()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AtFileHelper(param->arg,0,0,AddTitleFile);

    InitializeTDB();
    DumpIDDB(&title_db,stdout);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_test_options()
{
    printf("Options (hex=dec):\n");

    printf("  test:        %16d\n",testmode);
    printf("  verbose:     %16d\n",verbose);
    printf("  width:       %16d\n",opt_width);

 #if IS_WWT
    printf("  size:        %16llx = %lld\n",opt_size,opt_size);
    printf("  hd sec-size: %16x = %d\n",opt_hss,opt_hss);
    printf("  wb sec-size: %16x = %d\n",opt_wss,opt_wss);
    printf("  repair-mode: %16x = %d\n",repair_mode,repair_mode);
 #else
    u64 opt_size = 0;
 #endif

    printf("  chunk-mode:  %16x = %d\n",opt_chunk_mode,opt_chunk_mode);
    printf("  chunk-size:  %16x = %d%s\n",
		opt_chunk_size, opt_chunk_size,
		force_chunk_size ? " FORCE!" : "" );
    printf("  max-chunks:  %16x = %d\n",opt_max_chunks,opt_max_chunks);
    {
	u64 filesize[] = { 100ull*MiB, 1ull*GiB, 10ull*GiB, opt_size, 0 };
	u64 *fs_ptr = filesize;;
	for (;;)
	{
	    u32 n_blocks;
	    u32 block_size = CalcBlockSizeCISO(&n_blocks,*fs_ptr);
	    printf("    size->CISO %16llx = %12lld -> %5u * %8x/hex == %12llx/hex\n",
			*fs_ptr, *fs_ptr, n_blocks, block_size,
			(u64)block_size * n_blocks );
	    if (!*fs_ptr++)
		break;
	}
    }

    printf("  split-size:  %16llx = %lld\n",opt_split_size,opt_split_size);
    printf("  compression: %16x = %d = %s (level=%d)\n",
		opt_compr_method, opt_compr_method,
		wd_get_compression_name(opt_compr_method,"?"), opt_compr_level );
    printf("    level:     %16x = %d\n",opt_compr_level,opt_compr_level);
    printf("    chunk-size:%16x = %d\n",opt_compr_chunk_size,opt_compr_chunk_size);
    printf("  escape-char: %16x = %d\n",escape_char,escape_char);
    printf("  print-time:  %16x = %d\n",opt_print_time,opt_print_time);
    printf("  sort-mode:   %16x = %d\n",sort_mode,sort_mode);
    printf("  show-mode:   %16x = %d\n",opt_show_mode,opt_show_mode);
    printf("  limit:       %16x = %d\n",opt_limit,opt_limit);
    printf("  rdepth:      %16x = %d\n",opt_recurse_depth,opt_recurse_depth);
    printf("  enc:         %16x = %d\n",encoding,encoding);
    printf("  region:      %16x = %d\n",opt_region,opt_region);

    if (opt_ios_valid)
    {
	const u32 hi = opt_ios >> 32;
	const u32 lo = (u32)opt_ios;
	if ( hi == 1 && lo < 0x100 )
	    printf("  ios:        %08x-%08x = IOS %u\n", hi, lo, lo );
	else
	    printf("  ios:        %08x-%08x\n", hi, lo );
    }

    printf("  modify:      %16x = %d\n",opt_modify,opt_modify);
    if (modify_id)
	printf("  modify id:   '%s'\n",modify_id);
    if (modify_name)
	printf("  modify name: '%s'\n",modify_name);

 #if IS_WWT
    char buf_set_time[20];
    if (opt_set_time)
    {
	struct tm * tm = localtime(&opt_set_time);
	strftime(buf_set_time,sizeof(buf_set_time),"%F %T",tm);
    }
    else
	strcpy(buf_set_time,"NULL");
    printf("  set-time:    %16llx = %lld = %s\n",
		(u64)opt_set_time, (u64)opt_set_time,buf_set_time );
 #endif

    printf("  trim:        %16x = %u\n",opt_trim,opt_trim);
    printf("  align:       %16x = %u, %x = %u, %x = %u\n",
		opt_align1, opt_align1,
		opt_align2, opt_align2,
		opt_align3, opt_align3 );
    printf("  align-part:  %16x = %u = %s\n",
		opt_align_part, opt_align_part,
		wd_print_size(0,0,opt_align_part,true) );
    printf("  disc-size:   %16llx = %llu = %s\n",
		opt_disc_size, opt_disc_size,
		wd_print_size(0,0,opt_disc_size,true) );

    printf("  partition selector:\n");
    wd_print_select(stdout,6,&part_selector);

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError PrintErrorStat ( enumError err, ccp cmdname )
{
    if ( print_sections )
    {
	putchar('\n');
	if ( err )
	    printf("[error]\nprogram=%s\ncommand=%s\ncode=%u\nname=%s\ntext=%s\n\n",
			progname, cmdname, err, GetErrorName(err), GetErrorText(err) );
    }

    if ( err && verbose > 0 || err == ERR_NOT_IMPLEMENTED )
	fprintf(stderr,"%s: Command '%s' returns with status #%d [%s]\n",
			progname, cmdname, err, GetErrorName(err) );

    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
