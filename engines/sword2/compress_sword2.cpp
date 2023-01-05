/* ScummVM Tools
 *
 * ScummVM Tools is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/* Compress Broken Sword II sound clusters into MP3/Ogg Vorbis */

#include "compress_sword2.h"

#define TEMP_IDX	"tempfile.idx"
#define TEMP_DAT	"tempfile.dat"

uint32 CompressSword2::append_to_file(Common::File &f1, const char *filename) {
	uint32 length, orig_length;
	size_t size;
	char fbuf[2048];

	Common::File f2(filename, "rb");
	orig_length = length = f2.size();

	while (length > 0) {
		size = f2.read_noThrow(fbuf, length > sizeof(fbuf) ? sizeof(fbuf) : length);
		if (size <= 0) {
			break;
		}

		length -= size;
		f1.write(fbuf, size);
	}
	return orig_length;
}

#define GetCompressedShift(n)      ((n) >> 4)
#define GetCompressedSign(n)       (((n) >> 3) & 1)
#define GetCompressedAmplitude(n)  ((n) & 7)

CompressSword2::CompressSword2(const std::string &name) : CompressionTool(name, TOOLTYPE_COMPRESSION) {
	_supportsProgressBar = true;

	ToolInput input;
	input.format = "*.clu";
	_inputPaths.push_back(input);

	_shorthelp = "Used to compress Broken Sword 2 data files.";
	_helptext = "\nUsage: " + getName() + " [params] <file>\n\n";
}

void CompressSword2::execute() {
	int j;
	uint32 indexSize;
	uint32 totalSize;
	uint32 length;

	Common::Filename inpath(_inputPaths[0].path);
	Common::Filename &outpath = _outputPath;

	if (outpath.empty())
		// Extensions change between the in/out files, so we can use the same directory
		outpath = inpath;
	else if (outpath.directory())
		outpath.setFullName(inpath.getFullName());

	switch (_format) {
	case AUDIO_MP3:
		_audioOutputFilename = TEMP_MP3;
		outpath.setExtension(".cl3");
		break;
	case AUDIO_VORBIS:
		_audioOutputFilename = TEMP_OGG;
		outpath.setExtension(".clg");
		break;
	case AUDIO_FLAC:
		_audioOutputFilename = TEMP_FLAC;
		outpath.setExtension(".clf");
		break;
	default:
		throw ToolException("Unknown audio format");
		break;
	}

	_input.open(inpath, "rb");

	indexSize = _input.readUint32LE();
	totalSize = 12 * (indexSize + 1);

	if (_input.readUint32BE() != 0xfff0fff0) {
		error("This doesn't look like a music or speech cluster file");
	}

	_output_idx.open(TEMP_IDX, "wb");
	_output_snd.open(TEMP_DAT, "wb");

	_output_idx.writeUint32LE(indexSize);
	_output_idx.writeUint32BE(0xfff0fff0);
	_output_idx.writeUint32BE(0xfff0fff0);

	for (int i = 0; i < (int)indexSize; i++) {
		// Update progress, this loop is where most of the time is spent
		updateProgress(i, indexSize);

		uint32 pos;
		uint32 enc_length;

		_input.seek(8 * (i + 1), SEEK_SET);

		pos = _input.readUint32LE();
		length = _input.readUint32LE();
		
		/*
		* -- ner0 --
		* Filtering or replacing speech and music items by position
		*
		* - English (PC Gamer Demo):
		* 	md5: 6b8821c47228d4769f2466ba50c48152 *SPEECH.CLU
		* 	md5: 7421e0ba8b8ace92137699962097fb88 *MUSIC.CLU
		* 	GENERAL.CLU: 522ecd261027f0b55315a32aaef97295 (first 5000 bytes), 4519015 bytes
		* 	TEXT.CLU: dd156f3a5deb677505a770d45b4a7de5 (first 5000 bytes), 324690 bytes
		*
		* - French (CD1):
		* 	md5: 9c3ebb5e8479658a7808406d2388440c *SPEECH.CLU
		* 	md5: b7fa8c50b3d6b7958b39df6e87bb79e2 *MUSIC.CLU
		* 	GENERAL.CLU: 31db8564f9187538f24d9fda0677f666 (first 5000 bytes), 7059728 bytes
		* 	TEXT.CLU: d0cafb4d2982613ca4cf0574a0e4e079 (first 5000 bytes), 418165 bytes
		*/
		uint32 NEW_WAV_pos = 50224034;
		NEW_WAV_pos = 0; // comment this if you want to use an external WAV
		std::string NEW_WAV_str = std::__cxx11::to_string(NEW_WAV_pos) + ".wav";
		const char *NEW_WAV = NEW_WAV_str.c_str();
		if (NEW_WAV_pos > 0 && pos == NEW_WAV_pos) {
			// Replace item with external WAV file named with position number
			Common::File NEW_WAV_fp(NEW_WAV, "rb");
			length = ((NEW_WAV_fp.size() - 36) / 2) - 3;
		} else if (
			// List of positions for items to keep/skip
			/*
			// ### SPEECH.CLU ###
			(pos < 66001574 || pos > 67141515) 			// dock watchman convo
			&& (pos < 67490328 || pos > 73930653) 		// dock environment speech
			&& (pos < 110240700 || pos > 110881173) 	// dog biscuits, hook stick, beer bottle
			&& pos != 111469348 						// lucky piece of coal
			&& pos != 111665834 						// chimney cone
			&& pos != 112456239 && pos != 112516115 	// poisoned dart
			&& pos != 113178062 && pos != 113240066 	// tequilla worm
			&& pos != 113779524 						// condor transglobal label
			&& pos != 113831139 && pos != 113972118 	// nico's lipstick
			&& pos != 114270358 						// eclipse news clipping
			&& pos != 115266855 && pos != 115346407 	// lobineau's letter
			&& pos != 116723411 						// oubier's bank statement
			&& pos != 117057811 						// red panties
			&& (pos < 152226453 || pos > 161141022) 	// dock environment speech
			&& pos != 174549555 && pos != 174677236 	// dock intro
			&& pos != 174864181 && pos != 174968630 	// dock outro
			/**/

			/**/
			// ### MUSIC.CLU ###
			pos != 121300354 	// menu / id 221
			&& pos != 9003697 	// inspect lobineau's letter / id 17
			&& pos != 6170808 	// inspect poisoned dart / id 10
			&& pos != 17418084 	// inspect red panties / id 30
			&& pos != 4642084 	// inspect red panties 2 / id 7
			&& pos != 40820296 	// intro pos_4392
			&& pos != 43935006 	// inspect cabin's chimney pos_3079976
			&& pos != 47695134 	// beer bottle on cabin's chimney pos_5375272
			&& pos != 41591070 	// approach cabin/fence pos_741672
			&& pos != 50224034 	// dog trap pos_7904172 [pos_50224034 from CD is different, use pos_7904172 WAV from demo]
			&& pos != 41940254 	// peek cabin from window pos_1085224
			&& pos != 42475806 	// ask watchman about time pos_1620776
			&& pos != 43010334 	// ask watchman about condor transglobal pos_2155304
			&& pos != 43525406 	// ominous pos_2670376 
			&& pos != 44341022 	// grab beer bottle pos_3485992
			&& pos != 46913822 	// peek cabin from trap door pos_4593960
			&& pos != 48765858 	// enter cabin from trap door pos_6445996
			&& pos != 47145246 	// fart fall pos_4825384
			&& pos != 48021278 	// fill cabin with smoke pos_5701416
			&& pos != 49945506 	// touch stove pos_7625644
			&& pos != 49285026 	// pick box of biscuits pos_6965164
			&& pos != 49582498 	// pick lucky piece of coal pos_7262636
			&& pos != 50615202 	// outro pos_8669100
			/**/
			
			// to ignore this list...
			&& 0 == 1 // comment this if you want to filter encoded items
		) {
			// Skip current item, add empty entry and go to next item
			_output_idx.writeUint32LE(0);
			_output_idx.writeUint32LE(0);
			_output_idx.writeUint32LE(0);
			continue;
		}

		if (pos != 0 && length != 0) {
			uint16 prev;

			Common::File f(TEMP_WAV, "wb");

			/*
			 * The number of decodeable 16-bit samples is one less
			 * than the length of the resource.
			 */

			length--;

			/*
			 * Back when this tool was written, encodeAudio() had
			 * no way of encoding 16-bit data, so it was simpler to
			 * output a WAV file.
			 */

			f.writeUint32BE(0x52494646);	/* "RIFF" */
			f.writeUint32LE(2 * length + 36);
			f.writeUint32BE(0x57415645);	/* "WAVE" */
			f.writeUint32BE(0x666d7420);	/* "fmt " */
			f.writeUint32LE(16);
			f.writeUint16LE(1);				/* PCM */
			f.writeUint16LE(1);				/* mono */
			f.writeUint32LE(22050);			/* sample rate */
			f.writeUint32LE(44100);			/* bytes per second */
			f.writeUint16LE(2);				/* basic block size */
			f.writeUint16LE(16);			/* sample width */
			f.writeUint32BE(0x64617461);	/* "data" */
			f.writeUint32LE(2 * length);

			_input.seek(pos, SEEK_SET);

			/*
			 * The first sample is stored uncompressed. Subsequent
			 * samples are stored as some sort of 8-bit delta.
			 */

			prev = _input.readUint16LE();

			f.writeUint16LE(prev);

			for (j = 1; j < (int)length; j++) {
				byte data;
				uint16 out;

				data = _input.readByte();
				if (GetCompressedSign(data))
					out = prev - (GetCompressedAmplitude(data) << GetCompressedShift(data));
				else
					out = prev + (GetCompressedAmplitude(data) << GetCompressedShift(data));

				f.writeUint16LE(out);
				prev = out;
			}

			f.close();

			if (NEW_WAV_pos > 0 && pos == NEW_WAV_pos) { //ner0
				encodeAudio(NEW_WAV, false, -1, tempEncoded, _format); //ner0
			} else { //ner0
				encodeAudio(TEMP_WAV, false, -1, tempEncoded, _format);
			} //ner0
			enc_length = append_to_file(_output_snd, tempEncoded);

			_output_idx.writeUint32LE(totalSize);
			_output_idx.writeUint32LE(length);
			_output_idx.writeUint32LE(enc_length);
			totalSize = totalSize + enc_length;
					
			/*
			* -- ner0 --
			* Rename WAV files with position number in filename to keep them after encoding
			*/
			/*
			std::string _audioOutputFilenameRename_str = "sword2_pos_" + std::__cxx11::to_string(pos) + ".wav";
			const char *_audioOutputFilenameRename = _audioOutputFilenameRename_str.c_str();
			rename(TEMP_WAV, _audioOutputFilenameRename);
			*/
		} else {
			_output_idx.writeUint32LE(0);
			_output_idx.writeUint32LE(0);
			_output_idx.writeUint32LE(0);
		}
	}

	_output_snd.close();
	_output_idx.close();

	Common::File output(outpath, "wb");

	append_to_file(output, TEMP_IDX);
	append_to_file(output, TEMP_DAT);

	output.close();

	Common::removeFile(TEMP_DAT);
	Common::removeFile(TEMP_IDX);
	Common::removeFile(TEMP_MP3);
	Common::removeFile(TEMP_OGG);
	Common::removeFile(TEMP_FLAC);
	Common::removeFile(TEMP_WAV);
}

#ifdef STANDALONE_MAIN
int main(int argc, char *argv[]) {
	CompressSword2 sword2(argv[0]);
	return sword2.run(argc, argv);
}
#endif

