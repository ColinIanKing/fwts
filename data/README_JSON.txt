README for JSON files for the FirmWare Test Suite (fwts)
========================================================

Special considerations for the JSON files:

olog.json

olog.json is automatically generated with a tool from the
skiboot git source:

git clone git://github.com/open-power/skiboot.git
cd skiboot
./external/fwts/generate-fwts-olog --output olog.json

If any changes are requested in the format or typos for olog.json,
please pursue the skiboot source as the mechanism for updating,
i.e. contact the skiboot maintainers to make the source code
change and then once the change is merged to the git tree then
the above tool can be used to generate a new olog.json file
which can then be submitted as the fwts source change.

The olog.json file is periodically submitted to fwts to refresh
the data based on the current skiboot source tree.

To contact the skiboot maintainers:
skiboot@lists.ozlabs.org
