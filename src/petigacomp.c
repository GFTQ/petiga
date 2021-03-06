#include "petiga.h"

#undef  __FUNCT__
#define __FUNCT__ "IGAComputeScalar"
/*@
   IGAComputeScalar - Evaluates a linear functional of a given vector

   Collective on IGA

   Input Parameters:
+  iga - the IGA context
.  vecU - the vector to be used in computing the scalars
.  n - the number of scalars being computed
.  Scalar - the function which represents the linear functional
-  ctx - user-defined context for evaluation routine (may be NULL)

   Output Parameter:
.  S - an array [0:n-1] of scalars produced by Scalar

   Details of Scalar:
$  PetscErrorCode Scalar(IGAPoint p,const PetscScalar *U,PetscInt n,PetscScalar *S,void *ctx);

+  p - point at which to evaluate the functional
.  U - the vector
.  n - the number of scalars being computed
.  S - an array [0:n-1] of scalars
-  ctx - [optional] user-defined context for evaluation routine

   Notes:
   This function can be used to evaluate linear functionals of the
   solution. Use this when you wish to compute errors in the energy
   norm or moments of the solution.

   Level: normal

.keywords: IGA, evaluating linear functional
@*/
PetscErrorCode IGAComputeScalar(IGA iga,Vec vecU,
                                PetscInt n,PetscScalar S[],
                                IGAFormScalar Scalar,void *ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = IGAComputeScalarCustom(iga,vecU,n,S,Scalar,ctx,PETSC_FALSE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#undef  __FUNCT__
#define __FUNCT__ "IGAComputeScalarCustom"
PetscErrorCode IGAComputeScalarCustom(IGA iga,Vec vecU,
                                      PetscInt n,PetscScalar S[],
                                      IGAFormScalar Scalar,void *ctx,
                                      PetscBool fix)
{
  MPI_Comm          comm;
  Vec               localU;
  const PetscScalar *arrayU;
  PetscScalar       *localS,*workS;
  IGAElement        element;
  IGAPoint          point;
  PetscScalar       *U;
  PetscErrorCode    ierr;
  PetscFunctionBegin;
  PetscValidHeaderSpecific(iga,IGA_CLASSID,1);
  PetscValidHeaderSpecific(vecU,VEC_CLASSID,2);
  PetscValidScalarPointer(S,3);
  IGACheckSetUp(iga,1);

  ierr = PetscCalloc1(n,&localS);CHKERRQ(ierr);
  ierr = PetscMalloc1(n,&workS);CHKERRQ(ierr);

  /* Get local vector U and array */
  ierr = IGAGetLocalVecArray(iga,vecU,&localU,&arrayU);CHKERRQ(ierr);

  ierr = PetscLogEventBegin(IGA_FormScalar,iga,vecU,0,0);CHKERRQ(ierr);

  /* Element loop */
  ierr = IGABeginElement(iga,&element);CHKERRQ(ierr);
  while (IGANextElement(iga,element)) {
    ierr = IGAElementGetValues(element,arrayU,&U);CHKERRQ(ierr);
    if (fix) { ierr = IGAElementFixValues(element,U);CHKERRQ(ierr); }
    /* Quadrature loop */
    ierr = IGAElementBeginPoint(element,&point);CHKERRQ(ierr);
    while (IGAElementNextPoint(element,point)) {
      ierr = PetscMemzero(workS,n*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = Scalar(point,U,n,workS,ctx);CHKERRQ(ierr);
      ierr = IGAPointAddArray(point,n,workS,localS);CHKERRQ(ierr);
    }
    ierr = IGAElementEndPoint(element,&point);CHKERRQ(ierr);
  }
  ierr = IGAEndElement(iga,&element);CHKERRQ(ierr);

  ierr = PetscLogEventEnd(IGA_FormScalar,iga,vecU,0,0);CHKERRQ(ierr);

  /* Restore local vector U and array */
  ierr = IGARestoreLocalVecArray(iga,vecU,&localU,&arrayU);CHKERRQ(ierr);

  /* Assemble global scalars S */
  ierr = IGAGetComm(iga,&comm);CHKERRQ(ierr);
  ierr = MPI_Allreduce(localS,S,n,MPIU_SCALAR,MPIU_SUM,comm);CHKERRQ(ierr);

  ierr = PetscFree(localS);CHKERRQ(ierr);
  ierr = PetscFree(workS);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}
