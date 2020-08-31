npm i -g moxygen

# Moxygen will fail if docs/docs.ms does not exist
mkdir -p docs/docs.ms

moxygen --anchors --output docs/docs.ms/api-docs.md docs/xml
