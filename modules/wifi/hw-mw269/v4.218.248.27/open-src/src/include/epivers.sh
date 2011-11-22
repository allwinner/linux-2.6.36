#! /bin/bash
#
# Create the epivers.h file from epivers.h.in
# 
# Epivers.h generation mechanism supports both cvs and svn based checkouts
#
# $Id: epivers.sh,v 13.39 2011-02-10 01:43:54 Exp $

# If the version header file already exists, increment its build number.
# Otherwise, create a new file.
if [ -f epivers.h ]; then
	build=`grep EPI_BUILD_NUMBER epivers.h | sed -e "s,.*BUILD_NUMBER[ 	]*,,"`
	build=`expr ${build} + 1`
	echo build=${build}
	sed -e "s,.*_BUILD_NUMBER.*,#define EPI_BUILD_NUMBER	${build}," \
		< epivers.h > epivers.h.new
	mv epivers.h epivers.h.prev
	mv epivers.h.new epivers.h
	exit 0

else # epivers.h doesn't exist

	BOM_PRODUCTTAG="../tools/release/producttag.txt"
	NULL="/dev/null"

	# Check for the in file, if not there we're in the wrong directory
	if [ ! -f epivers.h.in ]; then
		echo No epivers.h.in found
		exit 1
	fi

	# Rules for workspace identification
	# 1. Check if in SVN workspace via svn info or HeadURL keyword
	#    or existance of .svn subdir
	# 2. If not, if CVS subdir exists or none exist it is CVS workspace
	
	if [ -z "$VCTOOL" ]; then VCTOOL=cvs; fi
	
	svn info epivers.sh > $NULL 2>&1
	
	if [ "$?" == "0" ]; then
		VCTOOL=svn
		SVNURL=$(svn info epivers.sh | egrep "^URL:" 2> $NULL)
	else
		if [ -d "CVS" ];  then VCTOOL=cvs; fi
		if [ -d ".svn" ]; then VCTOOL=svn; fi
		# HeadURL is a SVN keyword property that is expanded if
		# property svn:keywords is set. Keyword is needed when 
		# epivers.* are copied to intermediate build directories
		# and loose their svn identities
		# NOTE: Temporarily display attached svn properties
		if [ "$VCTOOL" != "cvs" ]; then
			echo "SVN Keyword Properties on epivers.sh: \
			`svn propget svn:keywords epivers.sh 2> $NULL`"
		fi
		SVNURL='$HeadURL: http://svn.sj.broadcom.com/svn/wlansvn/proj/branches/ROMTERM_BRANCH_4_218/src/include/epivers.sh $'
		if echo "$SVNURL" | grep -q "HeadURL:.*http.*"; then
			VCTOOL=svn
		fi
	fi
	
	if echo "${TAG}" | grep -q "BRANCH\|TWIG"; then
		branchtag=$TAG
	else
		branchtag=""
	fi
	
	# If this is a tagged build, use the tag to supply the numbers
	# Tag should be in the form
	#    <NAME>_REL_<MAJ>_<MINOR>
	# or
	#    <NAME>_REL_<MAJ>_<MINOR>_RC<RCNUM>
	# or
	#    <NAME>_REL_<MAJ>_<MINOR>_RC<RCNUM>_<INCREMENTAL>

	if [ "$VCTOOL" == "cvs" ]; then
		# This keyword is updated by CVS upon checkout of epivers.sh
		CVSTAG='$Name: not supported by cvs2svn $'
		# Allow overriding the product tag from BOM config file
		if [ -r $BOM_PRODUCTTAG ]; then
			. $BOM_PRODUCTTAG
		fi
		# Remove leading cvs "Name: " and trailing " $"
		CVSTAG=${CVSTAG/#*: /}
		CVSTAG=${CVSTAG/% $/}
	else
		SRCBASE=..
		MERGERLOG=${SRCBASE}/../merger_sources.log
		# TODO: Post svn transition, network paths will be taken away
		GETCOMPVER=getcompver.py
		GETCOMPVER_NET=/projects/hnd_software/gallery/src/tools/build/$GETCOMPVER
		GETCOMPVER_NET_WIN=Z:${GETCOMPVER_NET}

		#
		# If there is a local copy GETCOMPVER use it ahead of network copy
		#
		if [ -s "$GETCOMPVER" ]; then
		        GETCOMPVER_PATH="$GETCOMPVER"
		elif [ -s "${SRCBASE}/../src/tools/build/$GETCOMPVER" ]; then
		        GETCOMPVER_PATH="${SRCBASE}/../src/tools/build/$GETCOMPVER"
		elif [ -s "$GETCOMPVER_NET" ]; then
		        GETCOMPVER_PATH="$GETCOMPVER_NET"
		elif [ -s "$GETCOMPVER_NET_WIN" ]; then
		        GETCOMPVER_PATH="$GETCOMPVER_NET_WIN"
		fi

		#
		# If $GETCOMPVER isn't found, fetch it from SVN
		# (this should be very rare)
		#
		if [ ! -s "$GETCOMPVER_PATH" ]; then
		        svn export -q \
               		 ^/proj/trunk/src/tools/build/${GETCOMPVER} \
		                ${GETCOMPVER} 2> $NULL
		        GETCOMPVER_PATH=$GETCOMPVER
		fi

		# Now get tag for src/include from automerger
		INC_TAG=$(python $GETCOMPVER_PATH $SRCBASE/../merger_sources.log src/include 2> /dev/null)
		if [ -n "${INC_TAG}" ]; then
			if [ "$SVNTAG" == "trunk" ]; then
				SVNTAG=$(date '+DUMMYTRUNK_REL_%Y_%m_%d')
			else
				SVNTAG=$INC_TAG
			fi
		else
			# If branch or tag svn proj dirs can't be identified
			# in SVNURL, then use date as SVNTAG
			case "${SVNURL}" in
				*/branches/*) 	SVNTAG=$(echo $SVNURL | sed -e 's%.*/branches/\(.*\)/src.*%\1%g' | xargs printf "%s")
						;;
				*/tags/*) 	SVNTAG=$(echo $SVNURL | sed -e 's%.*/tags/\(.*\)/src.*%\1%g' | xargs printf "%s")
						;;
				*/trunk/*) 	SVNTAG=$(date '+DUMMYTRUNK_REL_%Y_%m_%d')
						;;
				*)       	SVNTAG=$(date '+DUMMYSVN_REL_%Y_%m_%d')
						;;
			esac
			echo "SVNTAG String Derived = $SVNTAG"
		fi
	fi


	# TAG env var is supplied by calling makefile or build process
	#    
	# If the checkout is from a branch tag, cvs checkout or export does
	# not replace rcs keywords. In such instances TAG env variable can
	# be used (by uncommenting following line). TAG env variable format
	# itself needs to be validated for number of fields before being used.
	# (e.g: HEAD is not a valid tag, which results in all '0' values below)
	#
	VCTAG=${CVSTAG:-${SVNTAG}}

        if [ -n "$branchtag" ]; then
	   TAG=${TAG:-${VCTAG}}
        else
	   TAG=${VCTAG/HEAD/}
        fi

	# Split the tag into an array on underbar or whitespace boundaries.
	IFS="_	     " tag=(${TAG})
	unset IFS

        tagged=1
	if [ ${#tag[*]} -eq 0 ]; then
	   tag=(`date '+TOT REL %Y %m %d 0 %y'`);
	   # reconstruct a TAG from the date
	   TAG=${tag[0]}_${tag[1]}_${tag[2]}_${tag[3]}_${tag[4]}_${tag[5]}	   
	   tagged=0
	fi

	# Allow environment variable to override values.
	# Missing values default to 0
	#
	maj=${EPI_MAJOR_VERSION:-${tag[2]:-0}}
	min=${EPI_MINOR_VERSION:-${tag[3]:-0}}
	rcnum=${EPI_RC_NUMBER:-${tag[4]:-0}}

	# If increment field is 0, set it to date suffix if on TOB
	if [ -n "$branchtag" ]; then
		[ "${tag[5]:-0}" -eq 0 ] && echo "Using date suffix for incr"
		today=`date '+%Y%m%d'`
		incremental=${EPI_INCREMENTAL_NUMBER:-${tag[5]:-${today:-0}}}
	else
		incremental=${EPI_INCREMENTAL_NUMBER:-${tag[5]:-0}}
	fi
	origincr=${EPI_INCREMENTAL_NUMBER:-${tag[5]:-0}}
	build=${EPI_BUILD_NUMBER:-0}

	# Strip 'RC' from front of rcnum if present
	rcnum=${rcnum/#RC/}
	
	# strip leading zero off the number (otherwise they look like octal)
	maj=${maj/#0/}
	min=${min/#0/}
	rcnum=${rcnum/#0/}
	incremental=${incremental/#0/}
	origincr=${origincr/#0/}
	build=${build/#0/}

	# some numbers may now be null.  replace with with zero.
	maj=${maj:-0}
	min=${min:-0}

	rcnum=${rcnum:-0}
	incremental=${incremental:-0}
	origincr=${origincr:-0}
	build=${build:-0}

	if [ ${tagged} -eq 1 ]; then
	    # vernum is 32chars max
	    vernum=`printf "0x%02x%02x%02x%02x" ${maj} ${min} ${rcnum} ${origincr}`
	else 
	    vernum=`printf "0x00%02x%02x%02x" ${tag[7]} ${min} ${rcnum}`
	fi

	# make sure the size of vernum is under 32 bits. 
	# Otherwise, truncate. The string will keep full information.
	vernum=${vernum:0:10}

	# build the string directly from the tag, irrespective of its length
	# remove the name , the tag type, then replace all _ by . 
	tag_ver_str=${TAG/${tag[0]}_}
	tag_ver_str=${tag_ver_str/${tag[1]}_}
	tag_ver_str=${tag_ver_str//_/.}

	# record tag type
	tagtype=

	if [ "${tag[1]}" = "BRANCH" -o "${tag[1]}" = "TWIG" ]; then
	   tagtype=" (TOB)"
	fi

	echo version string: "$tag_ver_str"
	echo tag type:"$tagtype"

	if [ "$(uname -s)" == "Darwin" ]; then
	   # Mac does not like 2-digit numbers so convert the number to single
	   # digit. 5.100 becomes 5.1
	   if [ $min -gt 99 ]; then
	       minmac=`expr $min / 100`
	   else
	       minmac=$min
	   fi
	   epi_ver_dev="${maj}.${minmac}.0"
	else
	   epi_ver_dev="${maj}.${min}.${rcnum}"
	fi

	# OK, go do it
	echo "maj=${maj}, min=${min}, rc=${rcnum}, inc=${incremental}, build=${build}"

	sed \
		-e "s;@EPI_MAJOR_VERSION@;${maj};" \
		-e "s;@EPI_MINOR_VERSION@;${min};" \
		-e "s;@EPI_RC_NUMBER@;${rcnum};" \
		-e "s;@EPI_INCREMENTAL_NUMBER@;${incremental};" \
		-e "s;@EPI_BUILD_NUMBER@;${build};" \
		-e "s;@EPI_VERSION@;${maj}, ${min}, ${rcnum}, ${incremental};" \
		-e "s;@EPI_VERSION_STR@;${tag_ver_str};" \
		-e "s;@EPI_VERSION_TYPE@;${tagtype};" \
                -e "s;@EPI_VERSION_NUM@;${vernum};" \
		-e "s;@EPI_VERSION_DEV@;${epi_ver_dev};" \
		< epivers.h.in > epivers.h

fi # epivers.h
