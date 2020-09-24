npm i -g moxygen

# Moxygen will fail if doc/docs/docs.ms does not exist
mkdir -p doc/docs/docs.ms

moxygen --anchors --output doc/docs/docs.ms/api-docs.md doc/docs/xml
