import sys
import os
import string
import random

def usage():
    print("python " + __file__ + " [NUMBER_OF_WORDS]")

if len(sys.argv) != 2 :
    print("ERROR: wrong number of parameters!")
    usage()
    sys.exit(1)

random.seed(42)

number_of_words = int(sys.argv[1])
min_word_length = 2
max_word_length = 30

f_path = str(os.path.dirname(os.path.realpath(__file__))) + str("/../input_data/") + str("qsort_small_input.txt")
f = open(f_path, "w")

for i in range(number_of_words):
    f.write(''.join(random.choice(string.ascii_letters + string.digits) for _ in range(random.randint(min_word_length, max_word_length))))
    f.write("\n")