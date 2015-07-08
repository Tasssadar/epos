#!/bin/sed
/^[^ 	]* from [^ 	]*[ 	]*[^ 	]/h
/^	/H
${
x
p
}
