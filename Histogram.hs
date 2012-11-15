import System.Environment(getArgs, getProgName)
import System.Console.GetOpt
import Data.Maybe (fromMaybe)
import Data.Map (Map, insertWith, empty, findMin, findMax, lookup)

-- Arguments parsing
-- -----------------

-- Type for storing the cli flags
data Flag = BuckWidth Double deriving Show

-- Construction of the bucket width flag
buckWidthPrs :: String -> Flag
buckWidthPrs = BuckWidth . (read :: String -> Double)

-- The definition of all the options
options :: [OptDescr Flag]
options = [ Option ['w'] ["bucket-width"] (ReqArg buckWidthPrs "bucket-width") "bucket width" ]

-- Arguments' parsing function
parseArguments :: [String] -> IO ([Flag], [String])
parseArguments argv =
	case getOpt RequireOrder options argv of
		(o, [], [])	-> return (o, [])
		(_, _, errs)	-> ioError (userError(concat errs ++ usageInfo header options))
	where header = "histogram [OPTION...]"

-- Arguments interpretation
-- ------------------------

-- Semantic arguments definition structure
data Args = Args { buckWidth :: Double }

-- The default arguments structure
defArgs :: Args
defArgs = Args { buckWidth = 1.0 }

-- Function appending a flag to the structure
appFlag :: Args -> Flag -> Args
appFlag args (BuckWidth w) = args { buckWidth = w }

-- Flags processing function
procFlags :: [Flag] -> Args
procFlags = foldl appFlag defArgs

-- Data Processing
-- ---------------

type Histogram = Map Double Int

-- Computes the index of the bucket to which the biven value belongs
-- assuming the bucket width given by the buck argument.
buckIndFromVal :: Double -> Double -> Int
buckIndFromVal buck val = (floor :: Double -> Int) (val / buck + 0.5)

-- Takes a map, adds a value to it and returns the resulting map.
addValueToMap :: Double -> Histogram -> Double -> Histogram
addValueToMap buck hist val = let
	key = fromIntegral (buckIndFromVal buck val) * buck in
		insertWith (+) key 1 hist

-- Builds a string representing the result of the histogram analysis.
-- Apart from dusplaying bucket -> count associations it also provides
-- the entries for "empty buckets", which don't explicitly appear in
-- the histogram map object.
postprocess :: Double -> Histogram -> String
postprocess buck hist = let
	(min, _) = findMin hist
	(max, _) = findMax hist
	ctrs = [min, min + buck .. max]
	append = (\soFar key ->
		soFar ++ (case (Data.Map.lookup key hist) of
			Just v	-> (show key ++ "\t" ++ show v)
			Nothing	-> (show key ++ "\t" ++ "0")) ++ "\n") in

		foldl append [] ctrs

-- Main entry point.
main = do
	bareArgs <- getArgs
	(flags, _) <- parseArguments bareArgs
	contents <- getContents
	let
		Args { buckWidth = w } = procFlags flags
		values = map (read :: [Char] -> Double) $ lines contents
		histogram = foldl (addValueToMap w) (empty::Histogram) values in
			putStr (postprocess w histogram)
