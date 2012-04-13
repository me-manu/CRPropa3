#include "mpc/magneticField/turbulentMagneticFieldGrid.h"
#include "fftw3.h"

namespace mpc {

TurbulentMagneticFieldGrid::TurbulentMagneticFieldGrid(Vector3 origin,
		size_t samples, double spacing, double lMin, double lMax, double Brms,
		double powerSpectralIndex) :
		MagneticFieldGrid(origin, samples, spacing) {
	this->lMin = lMin;
	this->lMax = lMax;
	this->Brms = Brms;
	this->powerSpectralIndex = powerSpectralIndex;
	initialize();
}

void TurbulentMagneticFieldGrid::setSeed(int seed) {
	random.seed(seed);
	initialize();
}

void TurbulentMagneticFieldGrid::initialize() {
	size_t n = samples; // size of array
	size_t n2 = floor(n/2) + 1; // size array in z-direction in configuration space

	// arrays to hold the complex vector components of the B(k)-field
	fftw_complex *Bkx, *Bky, *Bkz;
	Bkx = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n * n * n2);
	Bky = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n * n * n2);
	Bkz = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n * n * n2);

	// calculate the n possible discrete wave numbers
	double K[n];
	for (int i = 0; i < n; i++)
		K[i] = (double) i / n - i / (n / 2);

	// construct the field in configuration space
	int i;
	double k, theta, phase, cosPhase, sinPhase;
	double kMin = spacing / lMax;
	double kMax = spacing / lMin;
	Vector3 b; // real b-field vector
	Vector3 ek, e1, e2; // orthogonal base
	Vector3 n0(1, 1, 1); // arbitrary vector to construct orthogonal base

	for (size_t ix = 0; ix < n; ix++) {
		for (size_t iy = 0; iy < n; iy++) {
			for (size_t iz = 0; iz < n2; iz++) {

				i = ix * n * n2 + iy * n2 + iz;
				ek.set(K[ix], K[iy], K[iz]);
				k = ek.mag();

				// wave outside of turbulent range -> B(k) = 0
				if ((k < kMin) || (k > kMax)) {
					Bkx[i][0] = 0;
					Bkx[i][1] = 0;
					Bky[i][0] = 0;
					Bky[i][1] = 0;
					Bkz[i][0] = 0;
					Bkz[i][1] = 0;
					continue;
				}

				// construct an orthogonal base ek, e1, e2
				if (ek.isParallel(n0, 1e-6)) {
					// ek parallel to (1,1,1)
					e1.set(-1., 1., 0);
					e2.set(1., 1., -2.);
				} else {
					// ek not parallel to (1,1,1)
					e1 = n0.cross(ek);
					e2 = ek.cross(e1);
				}
				e1 /= e1.mag();
				e2 /= e2.mag();

				// random orientation perpendicular to k
				theta = 2 * M_PI * random.rand();
				b = e1 * cos(theta) + e2 * sin(theta);

				// standard normal distributed amplitude weighted with k^alpha/2
				b *= random.randNorm() * pow(k, powerSpectralIndex / 2.);

				// uniform random phase
				phase = 2 * M_PI * random.rand();
				cosPhase = cos(phase); // real part
				sinPhase = sin(phase); // imaginary part

				Bkx[i][0] = b.x() * cosPhase;
				Bkx[i][1] = b.x() * sinPhase;
				Bky[i][0] = b.y() * cosPhase;
				Bky[i][1] = b.y() * sinPhase;
				Bkz[i][0] = b.z() * cosPhase;
				Bkz[i][1] = b.z() * sinPhase;
			}
		}
	}

	// in-place, complex to real, inverse Fourier transformation on each component
	// note that the last elements of B(x) are unused now
	double *Bx = (double*) Bkx;
	fftw_plan plan_x = fftw_plan_dft_c2r_3d(n, n, n, Bkx, Bx, FFTW_ESTIMATE);
	fftw_execute(plan_x);
	fftw_destroy_plan(plan_x);

	double *By = (double*) Bky;
	fftw_plan plan_y = fftw_plan_dft_c2r_3d(n, n, n, Bky, By, FFTW_ESTIMATE);
	fftw_execute(plan_y);
	fftw_destroy_plan(plan_y);

	double *Bz = (double*) Bkz;
	fftw_plan plan_z = fftw_plan_dft_c2r_3d(n, n, n, Bkz, Bz, FFTW_ESTIMATE);
	fftw_execute(plan_z);
	fftw_destroy_plan(plan_z);

	// calculate normalization
	double sumB2 = 0;
	for (size_t ix = 0; ix < n; ix++)
		for (size_t iy = 0; iy < n; iy++)
			for (size_t iz = 0; iz < n; iz++) {
				i = ix * n * 2*n2 + iy * 2*n2 + iz;
				sumB2 += pow(Bx[i], 2) + pow(By[i], 2) + pow(Bz[i], 2);
			}
	double weight = Brms / sqrt(sumB2 / (n * n * n));

	// normalize and save real component to the grid
	for (size_t ix = 0; ix < n; ix++)
		for (size_t iy = 0; iy < n; iy++)
			for (size_t iz = 0; iz < n; iz++) {
				i = ix * n * 2*n2 + iy * 2*n2 + iz;
				grid[ix][iy][iz] = Vector3(Bx[i], By[i], Bz[i])
						* weight;
			}

	fftw_free(Bkx);
	fftw_free(Bky);
	fftw_free(Bkz);
}

double TurbulentMagneticFieldGrid::getRMSFieldStrength() const {
	return Brms;
}

double TurbulentMagneticFieldGrid::getPowerSpectralIndex() const {
	return powerSpectralIndex;
}

double TurbulentMagneticFieldGrid::getCorrelationLength() const {
	double r = lMin / lMax;
	double a = -powerSpectralIndex - 2;
	return lMax / 2 * (a - 1) / a * (1 - pow(r, a)) / (1 - pow(r, a - 1));
}

} // namespace mpc
