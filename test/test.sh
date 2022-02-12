#!/bin/bash

status=0
successes=0
failures=0

record=0
if [ "$1" = '--record' ]; then
  record=1
  shift
fi

for testfile in "$@"; do
  objfile="${testfile%.vx}.o"
  outfile="${testfile%.vx}.out"
  stdoutfile="$testfile.stdout"

	./vxc "$testfile" > "$objfile"
  if [ $? -ne 0 ]; then
    echo "Compile error in $testfile"
    status=1
    ((failures += 1))
    continue
  fi

	ld "$objfile" --entry=0x401000 -o "$outfile"
  if [ $? -ne 0 ]; then
    echo "Link error in $testfile"
    status=1
    ((failures += 1))
    continue
  fi

  if [ $record = 1 ]; then
    "$outfile" | xxd > "$stdoutfile"
  else
    diff --color=auto --unified=0 --new-file "$stdoutfile" <("$outfile" | xxd)
    if [ $? = 0 ]; then
      ((successes += 1))
    else
      status=1
      ((failures += 1))
    fi
    if [ ! -f "$stdoutfile" ]; then
      echo "Missing $stdoutfile"
      status=1
    fi
  fi
done

if [ $record = 0 ]; then
  echo "$successes successes, $failures failures"
fi
exit $status
