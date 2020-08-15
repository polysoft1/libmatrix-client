#include <iostream>

#include "Messages.h"

const char *NEWLINE = "\n";
const char *TAB = "\t";

using LibMatrix::MessageBatch;
using LibMatrix::Message;

std::ostream& LibMatrix::operator<<(std::ostream& output, const LibMatrix::MessageBatch& in) {
	//Using NEWLINE to avoid flushing the buffer unnessarily
	output << in.roomId << NEWLINE;

	for(Message msg : in.messages) {
		output << TAB << msg.id << NEWLINE;

		output << TAB << TAB << msg.sender << NEWLINE;
		output << TAB << TAB << msg.content << NEWLINE;
		output << TAB << TAB << msg.timestamp << NEWLINE;
	}

	output << std::string("Next batch: ") << in.nextBatch << NEWLINE;
	return output << std::string("Previous batch: ") << in.prevBatch << NEWLINE;
}
