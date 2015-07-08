AC_DEFUN([EPOS_CXX_OPTION],
[AC_MSG_CHECKING([for $2], [epos_cv_cxx_opt_$1])
AC_CACHE_VAL(epos_cv_cxx_opt_$1,
[ cat > conftest.cc <<EOF
#include <stdio.h>
int main(int argc, char **argv)
{
	if (argc < 0 || ((int)argv) == 0) printf("");
	return 0;
}
EOF
epos_cv_cxx_opt_$1="unknown"
for opt in $3; do
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${opt} -o conftest conftest.cc 2>conftest2 1>&5 || continue
		msg=`cat conftest2`
		if test -z "$msg"; then
			epos_cv_cxx_opt_$1=$opt
			break
		fi
done
rm -f conftest conftest2 conftest.cc])
AC_MSG_RESULT(${epos_cv_cxx_opt_$1})
if test "x${epos_cv_cxx_opt_$1}" = "xunknown"; then
	$1=
else
	$1=${epos_cv_cxx_opt_$1}
fi])


