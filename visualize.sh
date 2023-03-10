#!/bin/sh
set -e

get_line() {
	[ $# -ne 2 ] && exit 1
	source_file="${1}"
	source_line="${2}"

	awk "NR == ${source_line} { print }" < "${source_file}"
}

if [ $# -ne 1 ]; then
	echo "USAGE: ${0##*/} EXECUTABLE" 1>&2
	exit 1
fi

EXECUTABLE="${1}"

awk '
/^Missed (true|false) branch at: / {
	print $5
}
' | sort -u | \
while read addr;
do
	out=$(addr2line -e "${EXECUTABLE}" "${addr}" | cut -d " " -f1)
	file=$(echo "${out}" | cut -d : -f1)
	line=$(echo "${out}" | cut -d : -f2)

	printf "[%s] %s:%d\t%s\n" "${addr}" "${file}" "${line}" "$(get_line "${file}" "${line}")"
done
