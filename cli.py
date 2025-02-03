#!/usr/bin/env python3
import os, sys, types, shlex, pathlib, subprocess, contextlib, collections, time, inspect, builtins, itertools

################################################################ Configuration ################################################################

def ROOT(*subpaths):
	return pathlib.Path(
		pathlib.Path(__file__).absolute().relative_to(pathlib.Path.cwd(), walk_up=True).parent,
		*subpaths
	)

TARGET_MCU = 'atmega328p'
USART_BAUD = 9600

COMPILER_SETTINGS = (
	# Miscellaneous flags.
	f'''
		-Os
		-std=gnu11
		-fmax-errors=1
		-fno-strict-aliasing
		-mmcu={TARGET_MCU}
	'''

	# Defines.
	f'''
	'''

	# Warning configuration.
	f'''
		-Werror
		-Wall
		-Wextra
		-Wswitch-enum
		-Wundef
		-Wfatal-errors
		-Wstrict-prototypes
		-Wshadow
		-Wswitch-default

		-Wno-unused-function
		-Wno-main
		-Wno-double-promotion
		-Wno-conversion
		-Wno-unused-variable
		-Wno-unused-parameter
		-Wno-comment
		-Wno-unused-but-set-variable
		-Wno-format-zero-length
		-Wno-unused-label
	'''

	# Search path.
	f'''{'\n'.join(f'-I "{ROOT(path)}"' for path in '''
		./build
	'''.split())}'''
)

################################################################ Helpers ################################################################

try:
	sys.path += [str(ROOT('./deps/MetaPreprocessor'))]
	import MetaPreprocessor
	Meta = MetaPreprocessor.Meta
except ImportError:
	sys.exit(
		'# Failed to import MetaPreprocessor; perhaps initialize the git submodule?\n'
		'#     git submodule update --init --recursive'
	)

def maxlen(xs):
	xs = list(xs)
	return max(len(x) for x in xs) if xs else 0

def round_up(x, n=1):
	return x + (n - x % n) % n

def lines_of(string):
	return [line.strip() for line in string.strip().splitlines()]

def execute(default=None, *, bash=None, cmd=None, pwsh=None, keyboard_interrupt_ok=False, error_ok=False):

	#
	# Determine the instructions to execute.
	#

	assert cmd is None or pwsh is None, 'CMD and PowerShell commands cannot be both provided.'

	use_powershell = False
	match sys.platform:

		case 'win32':
			if pwsh is None:
				insts          = cmd
				use_powershell = False
			else:
				insts          = pwsh
				use_powershell = True

		case _:
			insts = bash

	if insts is None:
		insts = default

	assert insts is not None, f'Missing shell instructions for OS: `{sys.platform}`.'

	#
	# Execute the instruction(s).
	#

	for inst in ([insts] if isinstance(insts, str) else insts):
		if inst is not None and (inst := inst.strip()):

			# Parse and format the line of shell command.
			lex                  = shlex.shlex(inst)
			lex.quotes           = '"'
			lex.whitespace_split = True
			lex.commenters       = ''
			inst                 = ' '.join(list(lex))

			# Display shell command to be executed.
			print(f'$ {inst}')

			# Invoke PowerShell; note that it's slow to do just this,
			# so we'd use CMD when we don't need PowerShell features.
			if use_powershell:
				inst = ['pwsh', '-Command', inst]

			# Execute shell command.
			try:
				if code := subprocess.call(inst, shell=True):
					if error_ok:
						return code
					else:
						sys.exit(f'# Command resulted in non-zero exit code: {code}.')
			except KeyboardInterrupt as err:
				if keyboard_interrupt_ok:
					pass
				else:
					raise err

################################################################ CLI Commands ################################################################

cli_commands = {}
def CLICommand(description):

	def decorator(function):

		global cli_commands

		# The parameter list of the function will be used to create the CLI command for the user to use.
		argspec = inspect.getfullargspec(function)
		names   = argspec.args     or []
		details = argspec.defaults or []

		# Parameter list of the function must follow a very specific syntax.
		assert (
			argspec.varargs        is None and
			argspec.varkw          is None and
			argspec.kwonlyargs     == []   and
			argspec.kwonlydefaults is None
		)

		# Parameter "help" is special and must be the first and only default-less one.
		if help := ('help' in names):
			assert names.index('help') == 0
			names = names[1:]

		# All parameters to the CLI command will be defined using the default values in the function's parameter list.
		parameters = []
		assert len(names) == len(details)
		for name, (kind, about) in zip(names, details):

			param = types.SimpleNamespace(
				name        = name,
				about       = about,
				has_default = False,
			)

			# If the kind is a tuple, then it's actually a kind-default pair.
			if isinstance(kind, tuple):
				param.has_default   = True
				kind, param.default = kind

			# The kind of parameter determines what type of value should be given for it.
			match kind:

				# The parameter is a specific type.
				case builtins.str:
					param.kind = kind

				# The parameter can be one of the values in the list.
				case list() as options:
					param.kind = options

				case unknown: assert False, unknown

			parameters += [param]

		# Keep a ledger of all CLI commands defined so far.
		assert function.__name__ not in cli_commands
		cli_commands[function.__name__] = types.SimpleNamespace(
			description = description,
			function    = function,
			parameters  = parameters,
			help        = help,
		)

		return function

	return decorator

@CLICommand(f'Delete the `{ROOT('build')}` folder.')
def clean():
	build = ROOT('build')
	execute(
		bash = f'''
			rm -rf {build}
		''',
		cmd = f'''
			if exist {build} rmdir /S /Q {build}
		''',
	)

@CLICommand('Compile and generate the binary for flashing.')
def build():

	################################ Meta-Preprocessing ################################

	#
	# Determine files to meta-preprocess.
	#

	metapreprocessor_file_paths = [
		str(pathlib.Path(root, file_name))
		for root, dirs, file_names in ROOT('./src').walk()
		for file_name in file_names
		if file_name.endswith(('.c', '.h'))
	]

	#
	# Begin meta-preprocessing!
	#

	try:
		MetaPreprocessor.do(
			output_dir_path    = str(ROOT('./build')),
			source_file_paths  = metapreprocessor_file_paths,
			additional_context = {
			},
		)
	except MetaPreprocessor.MetaError as err:
		sys.exit(
			f'{err}\n'
			f'\n'
			f'# See the above meta-preprocessor error.'
		)

	################################ Building ################################

	# Compile source code.
	execute(f'''
		avr-gcc
			{COMPILER_SETTINGS}
			-o {ROOT('./build/IttyBittyRTTY.elf')}
			{ROOT('./src/IttyBittyRTTY.c')}
	''')

	# Convert ELF into hex file.
	execute(f'''
		avr-objcopy
			-O ihex
			-j .text
			-j .data
			{ROOT('./build/IttyBittyRTTY.elf')}
			{ROOT('./build/IttyBittyRTTY.hex')}
	''')

def get_programmer_port(*, quiet, none_ok):

	import serial.tools.list_ports

	portids = [
		port.device
		for port in serial.tools.list_ports.comports()
		if (port.vid, port.pid) == (0x1A86, 0x7523) # "QinHeng Electronics CH340 serial converter".
	]

	match len(portids):

		case 0:
			if none_ok:
				return None
			else:
				sys.exit('# No port associated with an AVR programmer found.')

		case 1:
			if not quiet:
				print(f'# Using port for AVR programmer: `{portids[0]}`.')

			return portids[0]

		case _:
			sys.exit('# Multiple ports associated with an AVR programmer found.')

@CLICommand('Flash the binary to the MCU.')
def flash():
	execute(f'''
		avrdude
			-p {TARGET_MCU}
			-c arduino
			-V
			-P {get_programmer_port(quiet=True, none_ok=False)}
			-D -Uflash:w:"{pathlib.PosixPath(ROOT('./build/IttyBittyRTTY.hex'))}"
	''')

@CLICommand('Open the COM serial port of the ST-LINK.')
def talk():

	# The only reason why PowerShell is used here is because there's no convenient way
	# in Python to read a single character from STDIN with no blocking and buffering.
	# Furthermore, the serial port on Windows seem to be buffered up, so data before the
	# port is opened for reading is available to fetch; PySerial only returns data sent after
	# the port is opened.

	portid = get_programmer_port(quiet=True, none_ok=False)

	execute(
		bash = f'''
			picocom --baud={USART_BAUD} --quiet --imap=lfcrlf {portid}
		''',
		pwsh = f'''
			$port = new-Object System.IO.Ports.SerialPort {portid},{USART_BAUD},None,8,one;
			$port.Open();
			try {{
				while ($true) {{
					Write-Host -NoNewline $port.ReadExisting();
					if ([System.Console]::KeyAvailable) {{
						$port.Write([System.Console]::ReadKey($true).KeyChar);
					}}
					Start-Sleep -Milliseconds 1;
				}}
			}} finally {{
				$port.Close();
			}}
		''',
		# For closing the serial port.
		keyboard_interrupt_ok=True,
	)

@CLICommand(f'Show usage of `{ROOT(os.path.basename(__file__))}`.')
def help(
	specifically = ((str, None), 'Name of command to show help info on.'),
):
	print(f'Usage: {ROOT(os.path.basename(__file__))} [command]')

	#
	# Determine the lines of the help text.
	#

	ungrouped_infos = []

	for command_name, command in ([(specifically, cli_commands[specifically])] if specifically in cli_commands else cli_commands.items()):

		# Command description.
		ungrouped_infos += [('command', command_name, command.description)]

		# Parameter list.
		for param in command.parameters:

			ungrouped_infos += [('param', param)]

			match param.kind:

				# The type for the parameter can be assumed based on context.
				case builtins.str:
					pass

				# List out the valid values for the parameter.
				case list() as options:
					for option in options:
						ungrouped_infos += [('option', option)]

				case unknown: assert False, unknown

		# Have the command provide any additional help information.
		if command.help:
			command.function(help=ungrouped_infos)

	#
	# Format the help text so it'll look nice and readable.
	#

	if specifically in cli_commands:
		print()
		print('\t...')

	for type, infos in itertools.groupby(ungrouped_infos, lambda info: info[0]):

		infos = list(infos)
		just  = None

		def format_param(param):
			return f'[{param.name}{f' = {param.default}' if param.has_default else ''}]'

		match type:

			case 'command':
				print()
				just = maxlen(name for type, name, description in infos)

			case 'param':
				just = maxlen(format_param(param) for type, param in infos)

			case 'option':
				pass

			case 'action':
				just = maxlen(keystroke for type, keystroke, action in infos)

			case unknown: assert False, unknown

		for info in infos:
			match info:

				case ('command', name, description):
					print(f'\t{name.ljust(just)} | {description}')

				case ('param', param):
					print(f'\t\t{format_param(param).ljust(just)} {param.about}')

				case ('option', option):
					print(f'\t\t\t= {option}')

				case ('action', keystroke, action):
					print(f'\t\t{f'({keystroke})'.ljust(just + 2)} {action.description}')

				case unknown: assert False, unknown

	if specifically in cli_commands:
		print()
		print('\t...')

	print()

	if specifically is not None and specifically not in cli_commands:
		sys.exit(f'# Unknown command `{specifically}`.')

################################################################ Execute ################################################################

call = ' '.join([str(pathlib.Path(__file__).absolute().relative_to(pathlib.Path.cwd(), walk_up=True))] + sys.argv[1:])

if len(sys.argv) <= 1:
	help(specifically=None)

elif sys.argv[1] not in cli_commands:
	help(specifically=sys.argv[1])

else:
	command = cli_commands[sys.argv[1]]

	class UsageError(Exception):
		pass

	try:
		#
		# Parse the command line arguments.
		#

		parsed_args = {}

		if command.help: # We won't be needing the additional help information.
			parsed_args['help'] = None

		remaining_args = sys.argv[2:]

		for param in command.parameters:

			# Interpret the argument based on the kind of parameter we're currently processing.
			if remaining_args:

				parsed_arg_value = None
				match param.kind:

					case builtins.str:
						parsed_arg_value = remaining_args[0]

					case list() as options:

						option = remaining_args[0]

						if option not in options:
							raise UsageError(f'Invalid option for [{param.name}]: {remaining_args[0]}.')

						parsed_arg_value = option

					case unknown: assert False, unknown

				parsed_args[param.name] = parsed_arg_value
				remaining_args          = remaining_args[1:]

			# No more arguments, but the parameter we're on has a default anyways.
			elif param.has_default:
				parsed_args[param.name] = param.default

			# Input error!
			else:
				raise UsageError(f'Missing required argument: [{param.name}].')


		if remaining_args:
			raise UsageError(f'Unknown argument: {remaining_args[0]}.')

		#
		# Execute.
		#

		start = time.time()
		command.function(**parsed_args)
		end = time.time()
		print(f'# {call} : {end - start :.3f}s')

	except UsageError as e:
		help(specifically=sys.argv[1])
		sys.exit(f'# {e.args[0]}')
