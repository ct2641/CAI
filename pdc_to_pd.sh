#!/bin/bash

if [ $# -ne 1 ]
then
	echo usage: $0 file
	exit 1
elif [ ! -e $1 ]
then
	echo usage: $0 file
	exit 2
fi

tmpfile="pdc_to_pd_tmp_file_dont_use_this_gbh"
comma="thisiscomma"

cp $1 $tmpfile


# Get_Line pattern
Get_Line ()
{
	grep -i -n "$1" $tmpfile | sed 's/:.*//'
}

# Delete_Line line
Delete_Line ()
{
	sed -i "$1d" $tmpfile
}

# Remove_Spaces line
Remove_Spaces ()
{
	sed -i "$1s/\s//g" $tmpfile
}

# Change_To_Var_Array line type
Change_To_Var_Array ()
{
	sed -i "$1s/,/\",\"/g" $tmpfile
	sed -i "$1s/^/\t\"$2\" : [\"/" $tmpfile
	sed -i "$1s/$/\"] ,/" $tmpfile
}

# Change_Bounds range
Change_Bounds ()
{
	sed -i "$1s/^/\t\t\"/" $tmpfile
	sed -i "$1s/$/\" ,/" $tmpfile
}

# Find_Last_Line_Of_Section cur_line
Find_Last_Line_Of_Section ()
{
	list=$(Get_Line "Dependency\|Conditional independence\|Constant bounds\|Bounds to prove\|Symmetry\|^\s*end\s*$")
	for num in $list
	do
		if [ $num -gt $1 ]
		then
			echo $(($num-1))
			break
		fi
	done
}


# Remove comments and empty lines
Delete_Line '/\/\//'
Delete_Line '/^\s*$/'
sed -i "s/\r//g" $tmpfile

# PD
sed -i "1s/^/# Fake Comment\n\n\nPD\n{\n/" $tmpfile

# RVs
line=$(Get_Line "Random variables")
if [ ! -z $line ]
then
	Delete_Line $line
	Remove_Spaces $line
	Change_To_Var_Array $line "RV"
fi

# ALs
line=$(Get_Line "Additional LP variables")
if [ ! -z $line ]
then
	Delete_Line $line
	Remove_Spaces $line
	Change_To_Var_Array $line "AL"
fi

# O
line=$(Get_Line "Objective")
if [ ! -z $line ]
then
	Delete_Line $line
	sed -i "${line}s/^/\t\"O\"  : \"/" $tmpfile
	sed -i "${line}s/$/\" ,/" $tmpfile
fi



# D
line=$(Get_Line "Dependency")
if [ ! -z $line ]
then
	line=$(($line+1))
	lastline=$(Find_Last_Line_Of_Section $line)
	range="${line},${lastline}"
	Remove_Spaces $range
	sed -i "${range}s/:/\"] $comma \"given\" : [\"/" $tmpfile
	sed -i "${range}s/^/\t\t{\"dependent\" : [\"/" $tmpfile
	sed -i "${range}s/,/\",\"/g" $tmpfile
	sed -i "${range}s/$comma/,/" $tmpfile
	sed -i "${range}s/$/\"]} ,/" $tmpfile
	sed -i "${lastline}s/ ,$/\n\t\t],/" $tmpfile
	line=$(($line-1))
	sed -i "${line}s/$/\n\t\"D\"  : [/" $tmpfile
fi


# I
line=$(Get_Line "Conditional independence")
if [ ! -z $line ]
then
	line=$(($line+1))
	lastline=$(Find_Last_Line_Of_Section $line)
	range="${line},${lastline}"
	Remove_Spaces $range
	sed -i "${range}s/|\|$/\"] $comma \"given\" : [\"/" $tmpfile
	sed -i "${range}s/^/\t\t{\"independent\" : [\"/" $tmpfile
	sed -i "${range}s/,/\",\"/g" $tmpfile
	sed -i "${range}s/;/\",\"/g" $tmpfile
	sed -i "${range}s/$comma/,/" $tmpfile
	sed -i "${range}s/$/\"]} ,/" $tmpfile
	sed -i "${range}s/\"\"//" $tmpfile
	sed -i "${lastline}s/ ,$/\n\t\t],/" $tmpfile
	line=$(($line-1))
	sed -i "${line}s/$/\n\t\"I\"  : [/" $tmpfile
fi


# BC
line=$(Get_Line "Constant bounds")
if [ ! -z $line ]
then
	line=$(($line+1))
	lastline=$(Find_Last_Line_Of_Section $line)
	range="${line},${lastline}"
	Change_Bounds $range
	sed -i "${lastline}s/ ,$/\n\t\t],/" $tmpfile
	line=$(($line-1))
	sed -i "${line}s/$/\n\t\"BC\" : [/" $tmpfile
fi


# BP
line=$(Get_Line "Bounds to prove")
if [ ! -z $line ]
then
	line=$(($line+1))
	lastline=$(Find_Last_Line_Of_Section $line)
	range="${line},${lastline}"
	Change_Bounds $range
	sed -i "${lastline}s/ ,$/\n\t\t],/" $tmpfile
	line=$(($line-1))
	sed -i "${line}s/$/\n\t\"BP\" : [/" $tmpfile
fi


# S
line=$(Get_Line "Symmetry")
if [ ! -z $line ]
then
	line=$(($line+1))
	lastline=$(Find_Last_Line_Of_Section $line)
	range="${line},${lastline}"
	Remove_Spaces $range
	sed -i "${range}s/^/\t\t[\"/" $tmpfile
	sed -i "${range}s/,/\",\"/g" $tmpfile
	sed -i "${range}s/$/\"] ,/" $tmpfile
	sed -i "${lastline}s/ ,$/\n\t\t],/" $tmpfile
	line=$(($line-1))
	sed -i "${line}s/$/\n\t\"S\"  : [/" $tmpfile
fi


for word in "Dependency" "Conditional independence" "Constant bounds" "Bounds to prove" "Symmetry" 
do
	line=$(Get_Line $word)
	if [ ! -z $line ]
	then
		Delete_Line $line
	fi
done

line=$(Get_Line "^\s*end\s*$")
sed -i "$(($line-1))s/\s*,$//" $tmpfile
sed -i "${line}s/.*/}/" $tmpfile


cat $tmpfile
rm -f $tmpfile

echo
echo
echo
echo
echo
echo
echo
echo
echo
echo
read -p "Press enter to continue"

