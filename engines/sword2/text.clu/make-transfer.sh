cd ~/scummvm-tools
rm ./*.clg & rm ./tempfile.* & rm ./sword2*.wav & make clean && make -j4
./scummvm-tools-cli --tool compress_sword2 --vorbis --silent speech.clu && smbclient //hostname.local/sharedfolder -c 'cd folder/subfolder ; put speech.clg' -U domain/username%password
./scummvm-tools-cli --tool compress_sword2 --vorbis --silent music.clu && smbclient //hostname.local/sharedfolder -c 'cd folder/subfolder ; put music.clg' -U domain/username%password
