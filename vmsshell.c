/*
** 2013 April 4
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains OpenVMS-specific routines for implementing the
** readline and history functions.  These are NOT threadsafe and
** definitely expect to be called from within the shell module only!
*/
#include "sqliteInt.h"
#if SQLITE_OS_VMS               /* This file is used for OpenVMS only */
#include <descrip.h>
#include <smgdef.h>
#include <smg$routines.h>
#include <ssdef.h>
#include <starlet.h>
#include <stdlib.h>
#include <str$routines.h>
#include <stsdef.h>
#if defined(VAX)
# include <sys$routines.h>
#endif
extern char *local_getline(char *, FILE *, int);

/* Private common SMG$ storage */
static int kb = 0;

void vms_read_history(char *zHistory){
  static const int recall_size = 100, flags = SMG$M_KEEP_CONTENTS;
  struct dsc$descriptor sLine = { 0, DSC$K_DTYPE_T, DSC$K_CLASS_S, 0};
  int i, status;
  FILE *in;

  status = smg$create_virtual_keyboard(&kb,0,0,0,&recall_size);
  if( !$VMS_STATUS_SUCCESS(status) ){
    sys$exit(status);
  }

  in = fopen(zHistory,"r","dna=SYS$LOGIN:.DAT");
  if( in==0 ){
    return;
  }

  for(i=1; i<=recall_size; i++){
    if( (sLine.dsc$a_pointer = local_getline(0,in,0))==0 ){
      break;
    }
    sLine.dsc$w_length = strlen(sLine.dsc$a_pointer);

    status = smg$replace_input_line(&kb,&sLine,&i,&flags);
    free(sLine.dsc$a_pointer);
    if( !$VMS_STATUS_SUCCESS(status) ){
      break;
    }
  }

  fclose(in);
}

void vms_write_history(char *zHistory){
  struct dsc$descriptor dLine = { 0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
  int i, status;
  FILE *out;

  out = fopen(zHistory,"a","SYS$LOGIN:.DAT");
  if( out==0 ){
    fprintf(stderr, "-- Writing history file %s failed\n", zHistory);
    return;
  }
  rewind(out);

  i = 1;
  do{
    status = smg$return_input_line(&kb, &dLine, 0, &i);
    if( $VMS_STATUS_SUCCESS(status) ){
      fprintf(out, "%.*s\n", dLine.dsc$w_length, dLine.dsc$a_pointer);
    }
  }while( $VMS_STATUS_SUCCESS(status) );

  str$free1_dx(&dLine);
  fclose(out);
}

/*
** This routine reads a line of text from SYS$INPUT, stores
** the text in memory obtained from malloc() and returns a pointer
** to the text.  NULL is returned at end of file, or if malloc()
** fails.
**
** This interface uses SMG to handle command line recall and command
** line editing.
*/
char *vms_readline(char *zPrompt){
  static struct dsc$descriptor dLine = { 0, DSC$K_DTYPE_T, DSC$K_CLASS_D, 0};
  struct dsc$descriptor sPrompt;
  int status;
  char *zLine = 0;

  sPrompt.dsc$b_dtype = DSC$K_DTYPE_T;
  sPrompt.dsc$b_class = DSC$K_CLASS_S;
  if( zPrompt && *zPrompt ){
    sPrompt.dsc$w_length = strlen(zPrompt);
    sPrompt.dsc$a_pointer = zPrompt;
  }else{
    sPrompt.dsc$w_length = 0;
    sPrompt.dsc$a_pointer = 0;
  }

  status = smg$read_composed_line(&kb,0,&dLine,&sPrompt);
  if( $VMS_STATUS_SUCCESS(status) ){
    zLine = malloc(dLine.dsc$w_length+1);
    if( zLine ){
      memcpy(zLine,dLine.dsc$a_pointer,dLine.dsc$w_length);
      zLine[dLine.dsc$w_length] = '\0';
    }
  }

  return zLine;
}

#endif /* SQLITE_OS_VMS */