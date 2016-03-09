/*
 * Copyright (C) 2005 by Grigoriy A. Sitkarev                            
 * sitkarev@komi.tgk-9.ru                                                  
 *                                                                         
 * This program is free software; you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation; either version 2 of the License, or     
 * (at your option) any later version.                                   
 *                                                                        
 * This program is distributed in the hope that it will be useful,       
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         
 * GNU General Public License for more details.                          
 *                                                                         
 * You should have received a copy of the GNU General Public License     
 * along with this program; if not, write to the                         
 * Free Software Foundation, Inc.,                                       
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

#include "iec104.h"

#define IECASDU_PARSE_FUNC(name)					\
int name(struct iec_object *obj, unsigned short *com_addr, int *n,		\
	unsigned char *cause, unsigned char *test, unsigned char *pn, size_t ioa_len,	\
	size_t ca_len, unsigned char *buf, size_t buflen)

#define ASDU_LEN(asdu_len, num, sq, name, ca_len, ioa_len)		\
	if (sq)								\
		asdu_len = ca_len + IEC_TYPEID_LEN +			\
			(num * sizeof(struct name)) + ioa_len;		\
	else								\
		asdu_len = ca_len + IEC_TYPEID_LEN +			\
			num * ((sizeof(struct name)) + ioa_len)

#define ASDU_ADDR(asdu_addr, ioa_len, addrv)				\
	asdu_addr = (ioa_len == 3 ? addrv : addrv & 0xFF);	
	
#define iecasdu_parse_type(objp, com_addrp, np, causep, testp, pnp, 	\
		ioa_len, ca_len, bufp, buflen, type_name, struct_name) 	\
	int i, asdu_len;						\
	unsigned short addr_cur, *addrp;					\
	struct iec_unit_id *unitp;					\
	struct type_name *typep;					\
	assert((ioa_len && ioa_len <= 3) && (ca_len && ca_len <= 2));	\
	unitp = (struct iec_unit_id *) bufp;				\
	ASDU_LEN(asdu_len, unitp->num, unitp->sq, type_name, ca_len, ioa_len);\
	if (asdu_len != buflen)	\
		return 1;						\
	addrp = (unsigned short *) ((unsigned char *) unitp + IEC_TYPEID_LEN);	\
	ASDU_ADDR(*com_addrp, ca_len, *addrp);				\
	addrp = (unsigned short *)((unsigned char *) unitp + ca_len + IEC_TYPEID_LEN);		\
	typep = (struct type_name *)((unsigned char *) unitp + ca_len + IEC_TYPEID_LEN + ioa_len);	\
	*np = unitp->num;						\
	*causep = unitp->cause;						\
	*testp = unitp->t;						\
	*pnp = unitp->pn;						\
	if (unitp->sq) {						\
		ASDU_ADDR(addr_cur, ioa_len, *addrp);			\
		for (i = 0; i < unitp->num; i++, objp++, typep++, addr_cur++) { \
			objp->ioa = addr_cur;				\
			objp->o.struct_name = *typep;			\
		}							\
	} else {							\
		for (i = 0; i < unitp->num; i++, objp++) {		\
			ASDU_ADDR(objp->ioa, ioa_len, *addrp);		\
			objp->o.struct_name = *typep;			\
			addrp = (unsigned short *) 			\
			((unsigned char *) typep + sizeof(struct type_name));	\
			typep = (struct type_name *)			\
			((unsigned char *) typep + sizeof(struct type_name) + ioa_len);\
		}							\
	}								


IECASDU_PARSE_FUNC(iecasdu_parse_type1)
{
	//iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
	//	ca_len, buf, buflen, iec_type1, type1);


	int i, asdu_len;
	unsigned short addr_cur, *addrp;
	struct iec_unit_id *unitp;
	struct iec_type1 *typep;
	assert((ioa_len && ioa_len <= 3) && (ca_len && ca_len <= 2));
	unitp = (struct iec_unit_id *) buf;
	if (unitp->sq)
		asdu_len = ca_len + IEC_TYPEID_LEN +(unitp->num * sizeof(struct iec_type1)) + ioa_len;
	else	
		asdu_len = ca_len + IEC_TYPEID_LEN +unitp->num * ((sizeof(struct iec_type1)) + ioa_len);

	if (asdu_len != buflen)	
		return 1;	
	addrp = (unsigned short *) ((unsigned char *) unitp + IEC_TYPEID_LEN);	
	ASDU_ADDR(*com_addr, ca_len, *addrp);
	addrp = (unsigned short *)((unsigned char *) unitp + ca_len + IEC_TYPEID_LEN);
	typep = (struct iec_type1 *)((unsigned char *) unitp + ca_len + IEC_TYPEID_LEN + ioa_len);	
	*n = unitp->num;
	*cause = unitp->cause;
	*test = unitp->t;
	*pn = unitp->pn;
	if (unitp->sq) {
		ASDU_ADDR(addr_cur, ioa_len, *addrp);
		for (i = 0; i < unitp->num; i++, obj++, typep++, addr_cur++) { 
			obj->ioa = addr_cur;
			obj->o.type1 = *typep;
		}
	} else {
		for (i = 0; i < unitp->num; i++, obj++) {
			ASDU_ADDR(obj->ioa, ioa_len, *addrp);
			obj->o.type1 = *typep;
			addrp = (unsigned short *) ((unsigned char *) typep + sizeof(struct iec_type1));	
			typep = (struct iec_type1 *)((unsigned char *) typep + sizeof(struct iec_type1) + ioa_len);
		}							
	}
	return 0;
}

IECASDU_PARSE_FUNC(iecasdu_parse_type7)
{
	iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
		ca_len, buf, buflen, iec_type7, type7);
	return 0;
}

IECASDU_PARSE_FUNC(iecasdu_parse_type9)
{
	iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
		ca_len, buf, buflen, iec_type9, type9);
	return 0;
}

IECASDU_PARSE_FUNC(iecasdu_parse_type11)
{
	iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
		ca_len, buf, buflen, iec_type11, type11);
	return 0;
}

IECASDU_PARSE_FUNC(iecasdu_parse_type13)
{
	iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
		ca_len, buf, buflen, iec_type13, type13);
	return 0;
}

IECASDU_PARSE_FUNC(iecasdu_parse_type30)
{
	iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
		ca_len, buf, buflen, iec_type30, type30);
	return 0;
}

IECASDU_PARSE_FUNC(iecasdu_parse_type33)
{
	iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
		ca_len, buf, buflen, iec_type33, type33);
	return 0;
}

IECASDU_PARSE_FUNC(iecasdu_parse_type34)
{
	iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
		ca_len, buf, buflen, iec_type34, type34);
	return 0;
}

IECASDU_PARSE_FUNC(iecasdu_parse_type35)
{
	iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
		ca_len, buf, buflen, iec_type33, type33);
	return 0;
}

IECASDU_PARSE_FUNC(iecasdu_parse_type36)
{
	iecasdu_parse_type(obj, com_addr, n, cause, test, pn, ioa_len, 
		ca_len, buf, buflen, iec_type36, type36);
	return 0;
}

/**
 * iecasdu_parse - parse ASDU unit
 * @param obj : array of information objects, MUST be at least IEC_OBJECT_MAX
 * @param type : returned type identifier (1-127)
 * @param com_addr : returned common address of ASDU
 * @param cnt : returned number of information objects in obj array
 * @param cause : returned cause identifier (0-63)
 * @param test : returned test bit (1=test, 0=not test)
 * @param pn : returned P/N bit (0=positive confirm, 1=negative confirm)
 * @param ioa_len : information object address length (1-2)
 * @param ca_len : common address length (1-2)
 * @param buf : buffer which contains unparsed ASDU
 * @param buflen : ASDU length
 * @return : 0 - success, 1 - incorrect ASDU, 2 - unknown ASDU type
 */
int iecasdu_parse(struct iec_object *obj, unsigned char *type, unsigned short *com_addr, 
	int *cnt, unsigned char *cause, unsigned char *test, unsigned char *pn, size_t ioa_len, 
	size_t ca_len, unsigned char *buf, size_t buflen)
{
	int ret = 0;
	struct iec_unit_id *unitp;
	
	unitp = (struct iec_unit_id *) buf;
	switch (unitp->type) {
	case M_SP_NA_1:
		*type = M_SP_NA_1;
		ret = iecasdu_parse_type1(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	break;
	case M_BO_NA_1:
		*type = M_BO_NA_1;
		ret = iecasdu_parse_type7(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	break;
	case M_ME_NA_1:
		*type = M_ME_NA_1;
		ret = iecasdu_parse_type9(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	break;
	case M_ME_NB_1:
		*type = M_ME_NB_1;
		ret = iecasdu_parse_type11(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	break;
	case M_ME_NC_1:
		*type = M_ME_NC_1;
		ret = iecasdu_parse_type13(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	break;
	case M_SP_TB_1:
		*type = M_SP_TB_1;
		ret = iecasdu_parse_type30(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	break;
	case M_BO_TB_1:
		*type = M_BO_TB_1;
		ret = iecasdu_parse_type33(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	break;
	case M_ME_TD_1:
		*type = M_ME_TD_1;
		ret = iecasdu_parse_type34(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	break;
	case M_ME_TE_1:
		*type = M_ME_TE_1;
		ret = iecasdu_parse_type35(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	break;
	case M_ME_TF_1:
		*type = M_ME_TF_1;
		ret = iecasdu_parse_type36(obj, com_addr, cnt, cause, test, pn, 
			ioa_len, ca_len, buf, buflen);
	default:
		ret = 2;
	break;
	}
	return ret;
}


enum frame_type frame_type(struct iechdr *h)
{
	if (!(h->raw[0] & 0x1))
	return FRAME_TYPE_I;
	else if (!(h->raw[0] & 0x2))
		return FRAME_TYPE_S;
	else
		return FRAME_TYPE_U; 
}

enum uframe_func uframe_func(struct iechdr *h)
{
	if (h->raw[0] & 0x4)
	return STARTACT;
	else if (h->raw[0] & 0x8)
		return STARTCON;
	else if (h->raw[0] & 0x10)
		return STOPACT;
	else if (h->raw[0] & 0x20)
		return STOPCON;
	else if (h->raw[0] & 0x40)
		return TESTACT;
	else
		return TESTCON;
}

char * uframe_func_to_string(enum uframe_func func)
{
	switch (func) {
case STARTACT:
	return "STARTACT";
case STARTCON:
	return "STARTCON";
case STOPACT:
	return "STOPACT";
case STOPCON:
	return "STOPCON";
case TESTACT:
	return "TESTACT";
case TESTCON:
	return "TESTCON";
default:
	return "UNKNOWN";
	}
}

char * frame_to_string(struct iechdr *h)
{
	switch (frame_type(h)) {
case FRAME_TYPE_I:
	return "I";
case FRAME_TYPE_S:
	return "S";
case FRAME_TYPE_U:
	return "U";
default:
	return "0";
	}
}