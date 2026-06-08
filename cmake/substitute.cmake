file(READ "${INPUT}" content)
string(REPLACE "${PLACEHOLDER}" "${VALUE}" content "${content}")
file(WRITE "${OUTPUT}" "${content}")
