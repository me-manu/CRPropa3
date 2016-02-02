#ifndef CRPROPA_ABSTRACT_OUTPUT_H
#define CRPROPA_ABSTRACT_OUTPUT_H

#include "crpropa/Module.h"

#include <bitset>

namespace crpropa {

/**
 @class Output
 @brief Configurable output base class.
 */
class Output: public Module {
protected:
	double lengthScale, energyScale;
	std::bitset<64> fields;
	bool oneDimensional, begun, ended;
	
	void modify();
public:
	enum OutputColumn {
		TrajectoryLengthColumn,
		RedshiftColumn,
		CurrentIdColumn,
		CurrentEnergyColumn,
		CurrentPositionColumn,
		CurrentDirectionColumn,
		SourceIdColumn,
		SourceEnergyColumn,
		SourcePositionColumn,
		SourceDirectionColumn,
		CreatedIdColumn,
		CreatedEnergyColumn,
		CreatedPositionColumn,
		CreatedDirectionColumn
	};

	enum OutputType {
		Trajectory1D,
		Trajectory3D,
		Event1D,
 		Event3D,
		Everything
	};

	Output();
	Output(OutputType outputtype);
	
	void setEnergyScale(double scale);
	void setLengthScale(double scale);

	void setOutputType(OutputType outputtype);
	void set(OutputColumn field, bool value);
	void enable(OutputColumn field);
	void disable(OutputColumn field);
	void enableAll();
	void disableAll();
	void set1D(bool value);

	void process(Candidate *candidate) const;
	void beginRun();
	void endRun();
	virtual std::string getDescription() const  = 0;
};

} // namespace crpropa

#endif // CRPROPA_ABSTRACT_OUTPUT_H
