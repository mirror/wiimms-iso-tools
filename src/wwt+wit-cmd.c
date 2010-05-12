
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
	    const bool print_header = !(used_options&OB_NO_HEADER);

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

enumError cmd_exclude()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AtFileHelper(param->arg,true,AddExcludeID);

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
	AtFileHelper(param->arg,true,AddTitleFile);

    InitializeTDB();
    DumpIDDB(&title_db,stdout);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError cmd_test_options()
{
    printf("Options (hex=dec):\n");

 #if IS_WWT
    printf("  size:        %16llx = %lld\n",opt_size,opt_size);
    printf("  hd sec-size: %16x = %d\n",opt_hss,opt_hss);
    printf("  wb sec-size: %16x = %d\n",opt_wss,opt_wss);
    printf("  repair-mode: %16x = %d\n",repair_mode,repair_mode);
 #endif

    printf("  split-size:  %16llx = %lld\n",opt_split_size,opt_split_size);
    printf("  escape-char: %16x = %d\n",escape_char,escape_char);
    printf("  print-time:  %16x = %d\n",opt_print_time,opt_print_time);
    printf("  sort-mode:   %16x = %d\n",sort_mode,sort_mode);
    printf("  enc:         %16x = %d\n",encoding,encoding);
    printf("  rdepth:      %16x = %d\n",opt_recurse_depth,opt_recurse_depth);

 #if IS_WWT
    char buf_set_time[20];
    if (opt_set_time)
    {
	struct tm * tm = localtime(&opt_set_time);
	strftime(buf_set_time,sizeof(buf_set_time),"%F %T",tm);
    }
    else
	strcpy(buf_set_time,"NULL");
    printf("  set-time:    %16llx = %lld = %s\n",(u64)opt_set_time,(u64)opt_set_time,buf_set_time);
 #endif

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
