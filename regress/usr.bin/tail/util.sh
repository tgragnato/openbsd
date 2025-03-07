# $OpenBSD: util.sh,v 1.1 2024/12/21 07:49:03 anton Exp $

wait_until() {
	local _i=0

	while [ "$_i" -lt 8 ]; do
		sh -x -c "$*" && return 0
		sleep 0.5
		_i="$((_i + 1))"
	done
	echo timeout
	return 1
}
