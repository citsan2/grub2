/* parttool.c - common dispatcher and parser for partition operations */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/normal.h>
#include <grub/device.h>
#include <grub/disk.h>
#include <grub/partition.h>
#include <grub/parttool.h>
#include <grub/command.h>

static struct grub_parttool *parts = 0;
static int curhandle = 0;
static grub_dl_t mymod;

int 
grub_parttool_register(const char *part_name, 
		       const grub_parttool_function_t func,
		       const struct grub_parttool_argdesc *args)
{
  struct grub_parttool *cur;
  int nargs = 0;

#ifndef GRUB_UTIL
  if (! parts)
    grub_dl_ref (mymod);
#endif

  cur = (struct grub_parttool *) grub_malloc (sizeof (struct grub_parttool));
  cur->next = parts;
  cur->name = grub_strdup (part_name);
  cur->handle = curhandle++;
  for (nargs = 0; args[nargs].name != 0; nargs++);
  cur->nargs = nargs;
  cur->args = (struct grub_parttool_argdesc *) 
    grub_malloc ((nargs + 1) * sizeof (struct grub_parttool_argdesc));
  grub_memcpy (cur->args, args, 
	       (nargs + 1) * sizeof (struct grub_parttool_argdesc));
  
  cur->func = func;
  parts = cur;
  return cur->handle;
}

void
grub_parttool_unregister (int handle)
{
  struct grub_parttool *prev = 0, *cur, *t;
  for (cur = parts; cur; )
    if (cur->handle == handle)
      {
	grub_free (cur->args);
	grub_free (cur->name);
	if (prev)
	  prev->next = cur->next;
	else
	  parts = cur->next;
	t = cur;
	cur = cur->next;
	grub_free (t);
      }
    else
      {
	prev = cur;
	cur = cur->next;
      }
#ifndef GRUB_UTIL
  if (! parts)
    grub_dl_unref (mymod);
#endif
}

static grub_err_t
grub_cmd_parttool (grub_command_t cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  grub_device_t dev;
  struct grub_parttool *cur, *ptool;
  int *parsed;
  int i, j;
  grub_err_t err = GRUB_ERR_NONE;

  if (argc < 2)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "too few arguments");

  if (args[0][0] == '(' && args[0][grub_strlen (args[0]) - 1] == ')')
    {
      args[0][grub_strlen (args[0]) - 1] = 0;
      dev = grub_device_open (args[0] + 1); 
      args[0][grub_strlen (args[0]) - 1] = ')';
    }
  else
    dev = grub_device_open (args[0]); 

  if (! dev)
    return grub_errno;

  if (! dev->disk)
    {
      grub_device_close (dev);
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "not a disk");
    }

  if (! dev->disk->partition)
    {
      grub_device_close (dev);
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "not a partition");
    }

  parsed = (int *) grub_malloc (argc * sizeof (int));
  grub_memset (parsed, 0, argc * sizeof (int));

  for (i = 1; i < argc; i++)
    if (!grub_strcmp (args[i], "help"))
      {
	int found = 0;
	for (cur = parts; cur; cur = cur->next)
	  if (! grub_strcmp (dev->disk->partition->partmap->name, cur->name))
	    {
	      struct grub_parttool_argdesc *curarg;
	      found = 1;
	      for (curarg = cur->args; curarg->name; curarg++)
		{
		  int spacing = 20;
		
		  spacing -= grub_strlen (curarg->name);
		  grub_printf ("%s", curarg->name);

		  switch (curarg->type)
		    {
		    case GRUB_PARTTOOL_ARG_BOOL:
		      grub_printf ("+/-");
		      spacing -= 3;
		      break;

		    case GRUB_PARTTOOL_ARG_VAL:		      
		      grub_printf ("=VAL");
		      spacing -= 4;
		      break;

		    case GRUB_PARTTOOL_ARG_END:
		      break;
		    }
		  while (spacing-- > 0)
		    grub_printf (" ");
		  grub_printf ("%s\n", curarg->desc);
		}
	    }
	if (! found)
	  grub_printf ("Sorry no parttool is available for %s\n", 
		       dev->disk->partition->partmap->name);
	return GRUB_ERR_NONE;
      }

  for (i = 1; i < argc; i++)
    if (! parsed[i])
      {
	struct grub_parttool_argdesc *curarg;
	struct grub_parttool_args *pargs;
	for (cur = parts; cur; cur = cur->next)
	  if (! grub_strcmp (dev->disk->partition->partmap->name, cur->name))
	    {
	      for (curarg = cur->args; curarg->name; curarg++)
		if (!grub_strncmp (curarg->name, args[i], 
				   grub_strlen (curarg->name))
		    && ((curarg->type == GRUB_PARTTOOL_ARG_BOOL 
			 && (args[i][grub_strlen (curarg->name)] == '+' 
			     || args[i][grub_strlen (curarg->name)] == '-'))
			|| (curarg->type == GRUB_PARTTOOL_ARG_VAL
			    && args[i][grub_strlen (curarg->name)] == '=')))
		    
		  break;
	      if (curarg->name)
		break;
	    }
	if (! cur)
	  return grub_error (GRUB_ERR_BAD_ARGUMENT, "unrecognised argument %s",
			     args[i]);
	ptool = cur;
	pargs = (struct grub_parttool_args *) 
	  grub_malloc (ptool->nargs * sizeof (struct grub_parttool_args));
	grub_memset (pargs, 0, 
		     ptool->nargs * sizeof (struct grub_parttool_args));
	for (j = i; j < argc; j++)
	  if (!parsed[j])
	    {
	      for (curarg = ptool->args; curarg->name; curarg++)
		if (!grub_strncmp (curarg->name, args[i], 
				   grub_strlen (curarg->name))
		    && ((curarg->type == GRUB_PARTTOOL_ARG_BOOL 
			 && (args[j][grub_strlen (curarg->name)] == '+' 
			     || args[j][grub_strlen (curarg->name)] == '-'))
			|| (curarg->type == GRUB_PARTTOOL_ARG_VAL
			    && args[j][grub_strlen (curarg->name)] == '=')))
		  {
		    parsed[j] = 1;
		    pargs[curarg - ptool->args].set = 1;
		    switch (curarg->type)
		      {
		      case GRUB_PARTTOOL_ARG_BOOL:
			pargs[curarg - ptool->args].bool 
			  = (args[j][grub_strlen (curarg->name)] != '-');
			break;

		      case GRUB_PARTTOOL_ARG_VAL:
			pargs[curarg - ptool->args].str 
			  = (args[j] + grub_strlen (curarg->name) + 1);
			break;
			
		      case GRUB_PARTTOOL_ARG_END:
			break;
		      }
		  }
	    }

	err = ptool->func (dev, pargs);
	grub_free (pargs);
	if (err)
	  break;
      }

  grub_device_close (dev);
  return err;
}

static grub_command_t cmd;

GRUB_MOD_INIT(parttool)
{
  (void)mod;			/* To stop warning. */
  mymod = mod;
  cmd = grub_register_command ("parttool", grub_cmd_parttool, 
			       "parttool PARTITION COMMANDS", 
			       "perform COMMANDS on partition."
			       " use parttool PARTITION help for the list "
			       " of available commands");
}

GRUB_MOD_FINI(parttool)
{
  grub_unregister_command (cmd);
}