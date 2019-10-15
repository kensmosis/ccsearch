# import pandas as pd
import numpy as np
import numpy.ctypeslib as ctl
import ctypes
import random
import signal
import struct
import argparse
import os.path
import regex as re
import sys
import signal
#from ccsapi import ccsapi

exec(open("ccsapi.py").read())

# Needed for parms
class parms:
	pass

# Necessary because python is an unholy pile of crap
def KErr(*args, **kwargs):
	print(*args, file=sys.stderr, **kwargs)

# Necessary because python is an unholy pile of crap
def KErrDie(*args, **kwargs):
	print(*args, file=sys.stderr, **kwargs)
	sys.exit()

def ParseCommandLine(mp):
	parser= argparse.ArgumentParser(description='Generate a random file for use in apitest.py or ccsearch')
	parser.add_argument('-f',help='Specify input file.  Must be in columns, delimited by single-delimiters (comma by default, otherwise see -d option).  May have a header (see -H option).  Blank lines are ignored as is anything after a #.  Columns are ID, Value, Cost, Feature membership 1..n (each of which specifies the feature group it is a member of or - for none or a : delimited list of groups if multiple (for nonpartition features)).  Note that there must be at least one feature.  Values and costs must be >-999998.  There must be <32768 items.  Feature groups for items must be >=1.  Mandatory.',type=str, required=True)
	parser.add_argument('-H',help='The input file has a header. Basically, we ignore the first line. Cannot be :',action='store_true',required=False,default=',')
	parser.add_argument('-d',help='Specify delimiter char for input file.  Note that adjacent delimiters are non concatenated (one delimiter between columns).  Default is comma.  For tab-delimited, type the word tab.',type=str, default= ',')
	parser.add_argument('-P',help='Specify which feature is the Primary one (see the algo description for details of what this means).  Features are numbered from 1..n based on the columns of the input file. Mandatory.',type=str, required=True)
	parser.add_argument('-G',help='Specify the number of items we must select from each primary feature group to create a collection.  This is of the form n1:n2:n3:....  Each ni>=0 and their sum is the collection size.  The total length of the :-delimited array is the number of primary groups. Mandatory.',type=str,required=True)
	parser.add_argument('--ispart',help='Identify feature n as a partition. This requires that every item have one and only value in the corresponding column. The argument is 1..n.  It is desirable but not mandatory for non-primary features to be partitions.',action='append')
	parser.add_argument('-V',help='Verbosity level.  0 runs without outputting anything except the output file (if requested) or failure-level errors. 1+ output various levels of debugging info.  Default is 0.',type=int,default=0)
	parser.add_argument('-C',help='Specify a constraint as t:n:m where t is mingrp or maxitem (type 0 or 1 constraint), n is the relevant feature number (1 is 1st feature), and m is the relevant count number.  mingrp means means there must be items from >=m groups for feature n, and maxitem means there must be <=m items in any one group of feature n.  Multiple constraints of either type may be added!',action='append')
	parser.add_argument('--maxcost',help='Specify the maximum total cost of items in a collections. Mandatory.',type=float,required=True)
	parser.add_argument('--ctol',help='Specify the algo collection tolerance, which drives how far below the best collection found, we keep collections.  Runs 0-1 with 0 keeping the best only and 1 keeping all.  Default is 0.2, meaning we allow collections of value above 80% of the best so far. ',type=float,default=0.2)
	parser.add_argument('--itol',help='Specify the algo individual cull tolerance.  This is used to determine how much better than an item other items of lesser or equal cost must be in order for that item to be discarded.  It runs from 0+, with 0 being the most aggressive cull and higher numbers being less aggressive.  Default is 0.5.',type=float, default=0.5)
	parser.add_argument('--ntol',help='Specify the algo number of extra individual items per group which must strictly be better than an item for it to be discarded.  0 requires the number of selections from that group, while higher numbers are added.  Default is 1.',type=int,default=1)
	parser.add_argument('--resnumb',help='Specify the block allocation size (in number of collections) used by the memory manager.  Rarely necessary to specify.  Default is 10000.'  ,type=int,default=10000)
	parser.add_argument('--maxres',help='Specify the maximum number of collections to return/keep.  If 0, no maximum.  Note that this has an effect even if no output is specified because it controls what we keep internally.  Default is 10000.',type=int,default=10000)
	parser.add_argument('--smode',help='Specify the search mode.  There are 4 choices based on 2 main decisions:  do we move from primary groups with the least combos to most or vice versa, and do we select combos of items within a group in order of decreasing value or increasing cost.  The choices are 1=  Fewest-to-most combinations / Decreasing Value,  2=  Most-to-fewest combinations / Decreasing Value, 3=  Fewest-to-most combinations / Increasing Cost, 4=  Most-to-fewest combinations / Increasing Cost.  Default is 1.',type=int, default=1)
	parser.add_argument('-o',help='Specify output file for results.  This will contain the best maxres (or fewer) collections obtained, along with their values.  If omitted, no results are output.  Use stdout for standard out.',type=str, required=False,default='')
	parser.add_argument('--mctol',help='To avoid rounding issues when checking a sum of costs against maxcost, we require that it be <= maxcost+mctol.  The default for mctol is 0.01, which is fine for integer costs.  However, for smaller discretizations it should be made smaller --- and for general float costs, it should be set to 0.  Default is 0.01.',type=float, default=0.01)

	c= parser.parse_args()

	mp.ifile= c.f
	if (not os.path.isfile(mp.ifile)): KErrDie("Input file does not exist")

	mp.hasheader= c.H

	mp.delim= c.d
	if (mp.delim == 'tab'): mp.delim= '\t'
	if (mp.delim == ''): KErrDie("Cannot use empty delimiter")
	if (len(mp.delim)!=1): KErrDie("Delimiter must be a single char")
	if (mp.delim == ':' or mp.delim =='#'): KErrDie("Cannot use : or # as a delimiter (the are reserved)")
	
	mp.primary= int(c.P)
	if (mp.primary<=0): KErrDie("Primary feature number must be >=1")

	mp.pfn= list()
	tx= re.split(':',c.G)
	csz= 0
	for i in tx:
		k= int(i)
		if (k<0): KErrDie("Each primary feature group selection count (-G option) must be >=0")
		mp.pfn.append(k)
		csz+= k
	if (not (csz>0)): KErrDie("Total collection size (sum of -G items) must be >0")

	mp.part= set()
	for i in c.ispart:
		k= int(i)
		if (k<=0):  KErrDie("Feature specified as partition is not valid (must be >=1)")
		mp.part.add(k)

	mp.debug= c.V
	if (mp.debug<0): mp.debug= 0

	mp.C= list()
	for i in c.C:
		x= re.split(':',i)
		if (len(x)!=3): KErrDie("Constraint spec invalid (must be of form t:n:m)")
		ct= x[0]
		cn= int(x[1])
		cm= int(x[2])
		if (cn<=0 or cm<=0): KErrDie("Constraint spec t:n:m must have n>=1 and m>=0")
		if (ct == 'mingrp'): mp.C.append([0,cn,cm])
		elif (ct == 'maxitem'): mp.C.append([1,cn,cm])
		else: KErrDie("Contraint t:n:m must have t= mingrp or maxitem")
	if (len(mp.C)==0 and mp.debug>0): KErr("Warning: No constraints specified")

	mp.maxcost= c.maxcost

	mp.ctol= float(c.ctol)
	if (mp.ctol<0 or mp.ctol>1): KErrDie("ctol must be [0,1]")

	mp.itol= float(c.itol)
	if (mp.itol<0): KErrDie("itol must be >=0")

	mp.ntol= int(c.ntol)
	if (mp.ntol<0): KErrDie("ntol must be >=0")

	mp.resnumb= int(c.resnumb)
	if (mp.resnumb<10): KErrDie("resnumb must be >=10")

	mp.maxres= int(c.maxres)
	if (mp.maxres<0): KErrDie("maxres must be >=0")

	mp.smode= int(c.smode)
	if (mp.smode<1 or mp.smode>4): KErrDie("Smode must be 1,2,3, or 4.")
	
	mp.ofile= c.o
	
	mp.mctol= float(c.mctol)


def VerifyFile(feats,items,vals,costs,prim,pfnn,sil):
	ni= len(items)
	nf= len(feats)
	rv= True
	if (len(vals)!=ni or len(costs)!=ni):
		KErr("Columns read from input file differ in length")
		rv= False
	if (prim>nf):
		KErr("Primary feature specified > max feature number")
		rv= False
	for i in feats:
		if (len(i)!=ni):
			KErr("Columns read from input file differ in length")
			rv= False
	iset= set()
	for i in items:
		if (i==""):
			KErr("Empty item id not allowed")
			rv= False
		if (i in iset):
			KErr("Duplicate item id not allowed")
			rv= False
		iset.add(i)
	for i in vals:
		if (not (i>-999998)):
			KErr("Values must be > -999998")
			rv= False
	for i in costs:
		if (not (i>-999998)):
			KErr("Costs must be > -999998")
			rv= False
	for i in feats:
		for j in i:
			if (len(j)==0):
				KErr("Cannot have blank feature spec for item.  Must use - if item is in no feature groups for that feature")
				rv= False
			for k in j:
				if (k<0):
					KErr("Feature group(s) for item must be >=1")
					rv= False
	for i in feats[prim-1]:
		for j in i:
			if (j>pfnn):
				KErr("Primary feature group spec for an item cannot exceed the number specified via the -G option!")
				rv= False
	return rv

def LoadFile(ifile,hasheader,delim,silent,feats,items,vals,costs):
	skipfirst= hasheader
	nc= 0
	rc= True
	i= 0
	with open(ifile,'r') as f:
		for l in f:
			l= l.strip()
			l= re.sub('\#.*','',l)
			l= l.strip()
			if (l==''): continue
			x= re.split(delim,l)
			if (nc==0):
				nc= len(x)
				if (nc<3):
					KErr("Error: First relevant line in input file has fewer than 3 columns.")
					rc= False
				items.clear()
				vals.clear()
				costs.clear()
				feats.clear()
				for ii in range(0,nc-3): feats.append([])
			elif (len(x)!=nc): 
				KErr("Error: Column count mismatch in input file line %d.  We have %d and expected %d" % (i,len(x),nc))
				rc= False
			if (skipfirst):
				skipfirst= False
				continue
			items.append(x[0])
			vals.append(float(x[1]))
			costs.append(float(x[2]))
			for j in range(0,len(feats)):
				z= re.split(':',x[3+j])
				z1= [] if (z=='-') else [int(k) for k in z]
				feats[j].append(z1)
			i+= 1
	if (not silent):
		print("Read input file %s with %d items and %d features" % (ifile,i,nc-3))
	return rc

def genfeaturetable(f):
	ni= len(f)		# Number of items

	# Determine max group number. Group numbers start with 1 here (0 in C++).  So mg is the number of groups as well here.
	mg= 0
	for i in f:
		for j in i:
			if (j>mg): mg= j

	# Create the object
	z= np.zeros([ni,mg],dtype=np.int32,order='C')

	# Populate the object
	for i in range(0,len(f)):
		for j in f[i]:
			z[i][j-1]= 1

	# Create the necessary api object
	zapi= (z.__array_interface__['data'][0] + np.arange(z.shape[0])*z.strides[0]).astype(np.intp)

	# Return both
	return [mg, z, zapi]

def Main():
	# Initialize the API
	ccsapi()

	# Read our parms
	mp= parms()
	ParseCommandLine(mp)

	# Read and verify the input data file
	feats= list()		# List of features (as lists)
	items= list()		# List of item IDs
	vals= list()		# List of item values
	costs= list()		# List of item costs
	if (not LoadFile(mp.ifile,mp.hasheader,mp.delim,(mp.debug==0),feats,items,vals,costs)): KErrDie("Failed to read input file")
	if (not VerifyFile(feats,items,vals,costs,mp.primary,len(mp.pfn),(mp.debug==0))): KerrDie("Data read from input file failed verification")
	
	# Perform some checks we couldn't do earlier
	ni= len(items)
	nf= len(feats)
	nc= len(mp.C)

	pset= set()
	for i in mp.part:
		if (i>nf): KErrDie("Feature specified as partition (via --ispart) is > max feature number")
	for x in mp.C:
		if (x[1]>nf): KErrDie("Feature specified in constraint exceeds maximum from input file!")

	# Pass the parms and spec to C++
	py_ccs_init_parms(mp.ctol,mp.itol,mp.ntol,mp.resnumb,mp.maxres,mp.smode)
	py_ccs_init_struct(nf,mp.primary-1,np.array(mp.pfn,dtype=np.int32),len(mp.pfn),ni,mp.maxcost,nc)
	py_ccs_set_maxcosttol(mp.mctol)

	# Pass the features to C++
	for i in range(0,len(feats)):
		[ngi,fi,fiapi]= genfeaturetable(feats[i])
		py_ccs_init_feature(i,ngi,ni,(1 if i in mp.part else 0),fiapi)

	# Pass the item costs and values to C++
	py_ccs_init_items(np.array(costs,dtype=np.float32), np.array(vals,dtype=np.float32))
	
	# Pass the constraints to C++
	for i in range(0,len(mp.C)):
		x= mp.C[i]
		foo= [x[1]-1,x[2]]
		py_ccs_set_constraint(i,x[0],2,np.array(foo,dtype=np.int32))

	# Prepare for execution of the search algo
	if (py_ccs_lock_and_load()<1): KErrDie("ERROR: lock and load failed")

	# Pass signals to C++.  This allows Ctrl-C to interrupt
	signal.signal(signal.SIGINT, signal.SIG_DFL)
	
	# Execute the search algo
	if (py_ccs_execute(mp.debug)<1): KErrDie("ERROR: execute failed")

	# If no output requested, we are done!
	if (mp.ofile == ''):
		py_ccs_release()
		sys.exit()

	# Obtain the result count (and initialize result iterator) and collection length since we will need these
	nr= py_ccs_prepres()
	clen= py_ccs_colllen()

	# Pick a block size for retrieving results, and allocate the relevant arrays
	ssize= 100000
	if (ssize>nr): ssize= nr
	resr= np.zeros([ssize,clen],dtype=np.uint32)
	resrapi= (resr.__array_interface__['data'][0] + np.arange(resr.shape[0])*resr.strides[0]).astype(np.intp)
	resm= np.zeros([ssize],dtype=np.float32)

	# Open the output file if need be
	ofh= ''
	if (mp.ofile != 'stdout'): ofh= open(mp.ofile,'w')

	# Loop over the retrieval blocks
	for i in range(0,nr,ssize):
		nr1= py_ccs_getres(ssize,resrapi,resm)
# e1[i]-s1[i])): KErrDie("Error retrieving results in range [%d,%d)" % (s1[i],e1[i]))

		# Loop over the results within a retrieval block
		for j in range(0,nr1):
			ss= ""

			# Loop over the items in a given result collection
			vvv= list()
			for k in range(0,clen):
				vvv.append(resr[j][k])
			vvv.sort()
			for k in range(0,len(vvv)):
				if (k>0): ss+= " "
				ss+= str(vvv[k])

			# Tack on the value
			ss+= " "+str(resm[j])
			ss+= "\n"

			# Write the result
			if (ofh==''): sys.stdout.write(ss)
			else: ofh.write(ss)
		
	# Tidy up
	if (ofh!=''): ofh.close()
	py_ccs_release()



Main()

