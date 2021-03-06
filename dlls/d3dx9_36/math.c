/*
 * Mathematical operations specific to D3DX9.
 *
 * Copyright (C) 2008 David Adam
 * Copyright (C) 2008 Luis Busquets
 * Copyright (C) 2008 Jérôme Gardou
 * Copyright (C) 2008 Philip Nilsson
 * Copyright (C) 2008 Henri Verbeet
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define NONAMELESSUNION

#include "config.h"
#include "wine/port.h"

#include "windef.h"
#include "wingdi.h"
#include "d3dx9_36_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

static const ID3DXMatrixStackVtbl ID3DXMatrixStack_Vtbl;

struct ID3DXMatrixStackImpl
{
  ID3DXMatrixStack ID3DXMatrixStack_iface;
  LONG ref;

  unsigned int current;
  unsigned int stack_size;
  D3DXMATRIX *stack;
};


/*_________________D3DXColor____________________*/

D3DXCOLOR* WINAPI D3DXColorAdjustContrast(D3DXCOLOR *pout, CONST D3DXCOLOR *pc, FLOAT s)
{
    TRACE("(%p, %p, %f)\n", pout, pc, s);

    pout->r = 0.5f + s * (pc->r - 0.5f);
    pout->g = 0.5f + s * (pc->g - 0.5f);
    pout->b = 0.5f + s * (pc->b - 0.5f);
    pout->a = pc->a;
    return pout;
}

D3DXCOLOR* WINAPI D3DXColorAdjustSaturation(D3DXCOLOR *pout, CONST D3DXCOLOR *pc, FLOAT s)
{
    FLOAT grey;

    TRACE("(%p, %p, %f)\n", pout, pc, s);

    grey = pc->r * 0.2125f + pc->g * 0.7154f + pc->b * 0.0721f;
    pout->r = grey + s * (pc->r - grey);
    pout->g = grey + s * (pc->g - grey);
    pout->b = grey + s * (pc->b - grey);
    pout->a = pc->a;
    return pout;
}

/*_________________Misc__________________________*/

FLOAT WINAPI D3DXFresnelTerm(FLOAT costheta, FLOAT refractionindex)
{
    FLOAT a, d, g, result;

    TRACE("costheta %f, refractionindex %f\n", costheta, refractionindex);

    g = sqrtf(refractionindex * refractionindex + costheta * costheta - 1.0f);
    a = g + costheta;
    d = g - costheta;
    result = (costheta * a - 1.0f) * (costheta * a - 1.0f) / ((costheta * d + 1.0f) * (costheta * d + 1.0f)) + 1.0f;
    result *= 0.5f * d * d / (a * a);

    return result;
}

/*_________________D3DXMatrix____________________*/

D3DXMATRIX * WINAPI D3DXMatrixAffineTransformation(D3DXMATRIX *out, FLOAT scaling, const D3DXVECTOR3 *rotationcenter,
        const D3DXQUATERNION *rotation, const D3DXVECTOR3 *translation)
{
    TRACE("out %p, scaling %f, rotationcenter %p, rotation %p, translation %p\n",
            out, scaling, rotationcenter, rotation, translation);

    D3DXMatrixIdentity(out);

    if (rotationcenter)
    {
        out->u.m[3][0] = -rotationcenter->x;
        out->u.m[3][1] = -rotationcenter->y;
        out->u.m[3][2] = -rotationcenter->z;
    }

    if (rotation)
    {
        FLOAT temp00, temp01, temp02, temp10, temp11, temp12, temp20, temp21, temp22;

        temp00 = 1.0f - 2.0f * (rotation->y * rotation->y + rotation->z * rotation->z);
        temp01 = 2.0f * (rotation->x * rotation->y + rotation->z * rotation->w);
        temp02 = 2.0f * (rotation->x * rotation->z - rotation->y * rotation->w);
        temp10 = 2.0f * (rotation->x * rotation->y - rotation->z * rotation->w);
        temp11 = 1.0f - 2.0f * (rotation->x * rotation->x + rotation->z * rotation->z);
        temp12 = 2.0f * (rotation->y * rotation->z + rotation->x * rotation->w);
        temp20 = 2.0f * (rotation->x * rotation->z + rotation->y * rotation->w);
        temp21 = 2.0f * (rotation->y * rotation->z - rotation->x * rotation->w);
        temp22 = 1.0f - 2.0f * (rotation->x * rotation->x + rotation->y * rotation->y);

        out->u.m[0][0] = scaling * temp00;
        out->u.m[0][1] = scaling * temp01;
        out->u.m[0][2] = scaling * temp02;
        out->u.m[1][0] = scaling * temp10;
        out->u.m[1][1] = scaling * temp11;
        out->u.m[1][2] = scaling * temp12;
        out->u.m[2][0] = scaling * temp20;
        out->u.m[2][1] = scaling * temp21;
        out->u.m[2][2] = scaling * temp22;

        if (rotationcenter)
        {
            FLOAT x, y, z;

            x = out->u.m[3][0];
            y = out->u.m[3][1];
            z = out->u.m[3][2];

            out->u.m[3][0] = x * temp00 + y * temp10 + z * temp20;
            out->u.m[3][1] = x * temp01 + y * temp11 + z * temp21;
            out->u.m[3][2] = x * temp02 + y * temp12 + z * temp22;
        }
    }
    else
    {
        out->u.m[0][0] = scaling;
        out->u.m[1][1] = scaling;
        out->u.m[2][2] = scaling;
    }

    if (rotationcenter)
    {
        out->u.m[3][0] += rotationcenter->x;
        out->u.m[3][1] += rotationcenter->y;
        out->u.m[3][2] += rotationcenter->z;
    }

    if (translation)
    {
        out->u.m[3][0] += translation->x;
        out->u.m[3][1] += translation->y;
        out->u.m[3][2] += translation->z;
    }

    return out;
}

D3DXMATRIX * WINAPI D3DXMatrixAffineTransformation2D(D3DXMATRIX *out, FLOAT scaling,
        const D3DXVECTOR2 *rotationcenter, FLOAT rotation, const D3DXVECTOR2 *translation)
{
    FLOAT tmp1, tmp2, s;

    TRACE("out %p, scaling %f, rotationcenter %p, rotation %f, translation %p\n",
            out, scaling, rotationcenter, rotation, translation);

    s = sinf(rotation / 2.0f);
    tmp1 = 1.0f - 2.0f * s * s;
    tmp2 = 2.0 * s * cosf(rotation / 2.0f);

    D3DXMatrixIdentity(out);
    out->u.m[0][0] = scaling * tmp1;
    out->u.m[0][1] = scaling * tmp2;
    out->u.m[1][0] = -scaling * tmp2;
    out->u.m[1][1] = scaling * tmp1;

    if (rotationcenter)
    {
        FLOAT x, y;

        x = rotationcenter->x;
        y = rotationcenter->y;

        out->u.m[3][0] = y * tmp2 - x * tmp1 + x;
        out->u.m[3][1] = -x * tmp2 - y * tmp1 + y;
    }

    if (translation)
    {
        out->u.m[3][0] += translation->x;
        out->u.m[3][1] += translation->y;
    }

    return out;
}

HRESULT WINAPI D3DXMatrixDecompose(D3DXVECTOR3 *poutscale, D3DXQUATERNION *poutrotation, D3DXVECTOR3 *pouttranslation, CONST D3DXMATRIX *pm)
{
    D3DXMATRIX normalized;
    D3DXVECTOR3 vec;

    TRACE("(%p, %p, %p, %p)\n", poutscale, poutrotation, pouttranslation, pm);

    /*Compute the scaling part.*/
    vec.x=pm->u.m[0][0];
    vec.y=pm->u.m[0][1];
    vec.z=pm->u.m[0][2];
    poutscale->x=D3DXVec3Length(&vec);

    vec.x=pm->u.m[1][0];
    vec.y=pm->u.m[1][1];
    vec.z=pm->u.m[1][2];
    poutscale->y=D3DXVec3Length(&vec);

    vec.x=pm->u.m[2][0];
    vec.y=pm->u.m[2][1];
    vec.z=pm->u.m[2][2];
    poutscale->z=D3DXVec3Length(&vec);

    /*Compute the translation part.*/
    pouttranslation->x=pm->u.m[3][0];
    pouttranslation->y=pm->u.m[3][1];
    pouttranslation->z=pm->u.m[3][2];

    /*Let's calculate the rotation now*/
    if ( (poutscale->x == 0.0f) || (poutscale->y == 0.0f) || (poutscale->z == 0.0f) ) return D3DERR_INVALIDCALL;

    normalized.u.m[0][0]=pm->u.m[0][0]/poutscale->x;
    normalized.u.m[0][1]=pm->u.m[0][1]/poutscale->x;
    normalized.u.m[0][2]=pm->u.m[0][2]/poutscale->x;
    normalized.u.m[1][0]=pm->u.m[1][0]/poutscale->y;
    normalized.u.m[1][1]=pm->u.m[1][1]/poutscale->y;
    normalized.u.m[1][2]=pm->u.m[1][2]/poutscale->y;
    normalized.u.m[2][0]=pm->u.m[2][0]/poutscale->z;
    normalized.u.m[2][1]=pm->u.m[2][1]/poutscale->z;
    normalized.u.m[2][2]=pm->u.m[2][2]/poutscale->z;

    D3DXQuaternionRotationMatrix(poutrotation,&normalized);
    return S_OK;
}

FLOAT WINAPI D3DXMatrixDeterminant(CONST D3DXMATRIX *pm)
{
    D3DXVECTOR4 minor, v1, v2, v3;
    FLOAT det;

    TRACE("(%p)\n", pm);

    v1.x = pm->u.m[0][0]; v1.y = pm->u.m[1][0]; v1.z = pm->u.m[2][0]; v1.w = pm->u.m[3][0];
    v2.x = pm->u.m[0][1]; v2.y = pm->u.m[1][1]; v2.z = pm->u.m[2][1]; v2.w = pm->u.m[3][1];
    v3.x = pm->u.m[0][2]; v3.y = pm->u.m[1][2]; v3.z = pm->u.m[2][2]; v3.w = pm->u.m[3][2];
    D3DXVec4Cross(&minor, &v1, &v2, &v3);
    det =  - (pm->u.m[0][3] * minor.x + pm->u.m[1][3] * minor.y + pm->u.m[2][3] * minor.z + pm->u.m[3][3] * minor.w);
    return det;
}

D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX *pout, FLOAT *pdeterminant, CONST D3DXMATRIX *pm)
{
    int a, i, j;
    D3DXMATRIX out;
    D3DXVECTOR4 v, vec[3];
    FLOAT det;

    TRACE("(%p, %p, %p)\n", pout, pdeterminant, pm);

    det = D3DXMatrixDeterminant(pm);
    if ( !det ) return NULL;
    if ( pdeterminant ) *pdeterminant = det;
    for (i=0; i<4; i++)
    {
        for (j=0; j<4; j++)
        {
            if (j != i )
            {
                a = j;
                if ( j > i ) a = a-1;
                vec[a].x = pm->u.m[j][0];
                vec[a].y = pm->u.m[j][1];
                vec[a].z = pm->u.m[j][2];
                vec[a].w = pm->u.m[j][3];
            }
        }
    D3DXVec4Cross(&v, &vec[0], &vec[1], &vec[2]);
    out.u.m[0][i] = pow(-1.0f, i) * v.x / det;
    out.u.m[1][i] = pow(-1.0f, i) * v.y / det;
    out.u.m[2][i] = pow(-1.0f, i) * v.z / det;
    out.u.m[3][i] = pow(-1.0f, i) * v.w / det;
   }

   *pout = out;
   return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixLookAtLH(D3DXMATRIX *pout, CONST D3DXVECTOR3 *peye, CONST D3DXVECTOR3 *pat, CONST D3DXVECTOR3 *pup)
{
    D3DXVECTOR3 right, rightn, up, upn, vec, vec2;

    TRACE("(%p, %p, %p, %p)\n", pout, peye, pat, pup);

    D3DXVec3Subtract(&vec2, pat, peye);
    D3DXVec3Normalize(&vec, &vec2);
    D3DXVec3Cross(&right, pup, &vec);
    D3DXVec3Cross(&up, &vec, &right);
    D3DXVec3Normalize(&rightn, &right);
    D3DXVec3Normalize(&upn, &up);
    pout->u.m[0][0] = rightn.x;
    pout->u.m[1][0] = rightn.y;
    pout->u.m[2][0] = rightn.z;
    pout->u.m[3][0] = -D3DXVec3Dot(&rightn,peye);
    pout->u.m[0][1] = upn.x;
    pout->u.m[1][1] = upn.y;
    pout->u.m[2][1] = upn.z;
    pout->u.m[3][1] = -D3DXVec3Dot(&upn, peye);
    pout->u.m[0][2] = vec.x;
    pout->u.m[1][2] = vec.y;
    pout->u.m[2][2] = vec.z;
    pout->u.m[3][2] = -D3DXVec3Dot(&vec, peye);
    pout->u.m[0][3] = 0.0f;
    pout->u.m[1][3] = 0.0f;
    pout->u.m[2][3] = 0.0f;
    pout->u.m[3][3] = 1.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixLookAtRH(D3DXMATRIX *pout, CONST D3DXVECTOR3 *peye, CONST D3DXVECTOR3 *pat, CONST D3DXVECTOR3 *pup)
{
    D3DXVECTOR3 right, rightn, up, upn, vec, vec2;

    TRACE("(%p, %p, %p, %p)\n", pout, peye, pat, pup);

    D3DXVec3Subtract(&vec2, pat, peye);
    D3DXVec3Normalize(&vec, &vec2);
    D3DXVec3Cross(&right, pup, &vec);
    D3DXVec3Cross(&up, &vec, &right);
    D3DXVec3Normalize(&rightn, &right);
    D3DXVec3Normalize(&upn, &up);
    pout->u.m[0][0] = -rightn.x;
    pout->u.m[1][0] = -rightn.y;
    pout->u.m[2][0] = -rightn.z;
    pout->u.m[3][0] = D3DXVec3Dot(&rightn,peye);
    pout->u.m[0][1] = upn.x;
    pout->u.m[1][1] = upn.y;
    pout->u.m[2][1] = upn.z;
    pout->u.m[3][1] = -D3DXVec3Dot(&upn, peye);
    pout->u.m[0][2] = -vec.x;
    pout->u.m[1][2] = -vec.y;
    pout->u.m[2][2] = -vec.z;
    pout->u.m[3][2] = D3DXVec3Dot(&vec, peye);
    pout->u.m[0][3] = 0.0f;
    pout->u.m[1][3] = 0.0f;
    pout->u.m[2][3] = 0.0f;
    pout->u.m[3][3] = 1.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX *pout, CONST D3DXMATRIX *pm1, CONST D3DXMATRIX *pm2)
{
    D3DXMATRIX out;
    int i,j;

    TRACE("(%p, %p, %p)\n", pout, pm1, pm2);

    for (i=0; i<4; i++)
    {
        for (j=0; j<4; j++)
        {
            out.u.m[i][j] = pm1->u.m[i][0] * pm2->u.m[0][j] + pm1->u.m[i][1] * pm2->u.m[1][j] + pm1->u.m[i][2] * pm2->u.m[2][j] + pm1->u.m[i][3] * pm2->u.m[3][j];
        }
    }

    *pout = out;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixMultiplyTranspose(D3DXMATRIX *pout, CONST D3DXMATRIX *pm1, CONST D3DXMATRIX *pm2)
{
    TRACE("%p, %p, %p)\n", pout, pm1, pm2);

    D3DXMatrixMultiply(pout, pm1, pm2);
    D3DXMatrixTranspose(pout, pout);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoLH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / w;
    pout->u.m[1][1] = 2.0f / h;
    pout->u.m[2][2] = 1.0f / (zf - zn);
    pout->u.m[3][2] = zn / (zn - zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f, %f, %f)\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / (r - l);
    pout->u.m[1][1] = 2.0f / (t - b);
    pout->u.m[2][2] = 1.0f / (zf -zn);
    pout->u.m[3][0] = -1.0f -2.0f *l / (r - l);
    pout->u.m[3][1] = 1.0f + 2.0f * t / (b - t);
    pout->u.m[3][2] = zn / (zn -zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterRH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f, %f, %f)\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / (r - l);
    pout->u.m[1][1] = 2.0f / (t - b);
    pout->u.m[2][2] = 1.0f / (zn -zf);
    pout->u.m[3][0] = -1.0f -2.0f *l / (r - l);
    pout->u.m[3][1] = 1.0f + 2.0f * t / (b - t);
    pout->u.m[3][2] = zn / (zn -zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixOrthoRH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f / w;
    pout->u.m[1][1] = 2.0f / h;
    pout->u.m[2][2] = 1.0f / (zn - zf);
    pout->u.m[3][2] = zn / (zn - zf);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovLH(D3DXMATRIX *pout, FLOAT fovy, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, fovy, aspect, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 1.0f / (aspect * tan(fovy/2.0f));
    pout->u.m[1][1] = 1.0f / tan(fovy/2.0f);
    pout->u.m[2][2] = zf / (zf - zn);
    pout->u.m[2][3] = 1.0f;
    pout->u.m[3][2] = (zf * zn) / (zn - zf);
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveFovRH(D3DXMATRIX *pout, FLOAT fovy, FLOAT aspect, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, fovy, aspect, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 1.0f / (aspect * tan(fovy/2.0f));
    pout->u.m[1][1] = 1.0f / tan(fovy/2.0f);
    pout->u.m[2][2] = zf / (zn - zf);
    pout->u.m[2][3] = -1.0f;
    pout->u.m[3][2] = (zf * zn) / (zn - zf);
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveLH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f * zn / w;
    pout->u.m[1][1] = 2.0f * zn / h;
    pout->u.m[2][2] = zf / (zf - zn);
    pout->u.m[3][2] = (zn * zf) / (zn - zf);
    pout->u.m[2][3] = 1.0f;
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterLH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f, %f, %f)\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f * zn / (r - l);
    pout->u.m[1][1] = -2.0f * zn / (b - t);
    pout->u.m[2][0] = -1.0f - 2.0f * l / (r - l);
    pout->u.m[2][1] = 1.0f + 2.0f * t / (b - t);
    pout->u.m[2][2] = - zf / (zn - zf);
    pout->u.m[3][2] = (zn * zf) / (zn -zf);
    pout->u.m[2][3] = 1.0f;
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterRH(D3DXMATRIX *pout, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f, %f, %f)\n", pout, l, r, b, t, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f * zn / (r - l);
    pout->u.m[1][1] = -2.0f * zn / (b - t);
    pout->u.m[2][0] = 1.0f + 2.0f * l / (r - l);
    pout->u.m[2][1] = -1.0f -2.0f * t / (b - t);
    pout->u.m[2][2] = zf / (zn - zf);
    pout->u.m[3][2] = (zn * zf) / (zn -zf);
    pout->u.m[2][3] = -1.0f;
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixPerspectiveRH(D3DXMATRIX *pout, FLOAT w, FLOAT h, FLOAT zn, FLOAT zf)
{
    TRACE("(%p, %f, %f, %f, %f)\n", pout, w, h, zn, zf);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 2.0f * zn / w;
    pout->u.m[1][1] = 2.0f * zn / h;
    pout->u.m[2][2] = zf / (zn - zf);
    pout->u.m[3][2] = (zn * zf) / (zn - zf);
    pout->u.m[2][3] = -1.0f;
    pout->u.m[3][3] = 0.0f;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixReflect(D3DXMATRIX *pout, CONST D3DXPLANE *pplane)
{
    D3DXPLANE Nplane;

    TRACE("(%p, %p)\n", pout, pplane);

    D3DXPlaneNormalize(&Nplane, pplane);
    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 1.0f - 2.0f * Nplane.a * Nplane.a;
    pout->u.m[0][1] = -2.0f * Nplane.a * Nplane.b;
    pout->u.m[0][2] = -2.0f * Nplane.a * Nplane.c;
    pout->u.m[1][0] = -2.0f * Nplane.a * Nplane.b;
    pout->u.m[1][1] = 1.0f - 2.0f * Nplane.b * Nplane.b;
    pout->u.m[1][2] = -2.0f * Nplane.b * Nplane.c;
    pout->u.m[2][0] = -2.0f * Nplane.c * Nplane.a;
    pout->u.m[2][1] = -2.0f * Nplane.c * Nplane.b;
    pout->u.m[2][2] = 1.0f - 2.0f * Nplane.c * Nplane.c;
    pout->u.m[3][0] = -2.0f * Nplane.d * Nplane.a;
    pout->u.m[3][1] = -2.0f * Nplane.d * Nplane.b;
    pout->u.m[3][2] = -2.0f * Nplane.d * Nplane.c;
    return pout;
}

D3DXMATRIX * WINAPI D3DXMatrixRotationAxis(D3DXMATRIX *out, const D3DXVECTOR3 *v, FLOAT angle)
{
    D3DXVECTOR3 nv;
    FLOAT sangle, cangle, cdiff;

    TRACE("out %p, v %p, angle %f\n", out, v, angle);

    D3DXVec3Normalize(&nv, v);
    sangle = sinf(angle);
    cangle = cosf(angle);
    cdiff = 1.0f - cangle;

    out->u.m[0][0] = cdiff * nv.x * nv.x + cangle;
    out->u.m[1][0] = cdiff * nv.x * nv.y - sangle * nv.z;
    out->u.m[2][0] = cdiff * nv.x * nv.z + sangle * nv.y;
    out->u.m[3][0] = 0.0f;
    out->u.m[0][1] = cdiff * nv.y * nv.x + sangle * nv.z;
    out->u.m[1][1] = cdiff * nv.y * nv.y + cangle;
    out->u.m[2][1] = cdiff * nv.y * nv.z - sangle * nv.x;
    out->u.m[3][1] = 0.0f;
    out->u.m[0][2] = cdiff * nv.z * nv.x - sangle * nv.y;
    out->u.m[1][2] = cdiff * nv.z * nv.y + sangle * nv.x;
    out->u.m[2][2] = cdiff * nv.z * nv.z + cangle;
    out->u.m[3][2] = 0.0f;
    out->u.m[0][3] = 0.0f;
    out->u.m[1][3] = 0.0f;
    out->u.m[2][3] = 0.0f;
    out->u.m[3][3] = 1.0f;

    return out;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationQuaternion(D3DXMATRIX *pout, CONST D3DXQUATERNION *pq)
{
    TRACE("(%p, %p)\n", pout, pq);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = 1.0f - 2.0f * (pq->y * pq->y + pq->z * pq->z);
    pout->u.m[0][1] = 2.0f * (pq->x *pq->y + pq->z * pq->w);
    pout->u.m[0][2] = 2.0f * (pq->x * pq->z - pq->y * pq->w);
    pout->u.m[1][0] = 2.0f * (pq->x * pq->y - pq->z * pq->w);
    pout->u.m[1][1] = 1.0f - 2.0f * (pq->x * pq->x + pq->z * pq->z);
    pout->u.m[1][2] = 2.0f * (pq->y *pq->z + pq->x *pq->w);
    pout->u.m[2][0] = 2.0f * (pq->x * pq->z + pq->y * pq->w);
    pout->u.m[2][1] = 2.0f * (pq->y *pq->z - pq->x *pq->w);
    pout->u.m[2][2] = 1.0f - 2.0f * (pq->x * pq->x + pq->y * pq->y);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationX(D3DXMATRIX *pout, FLOAT angle)
{
    TRACE("(%p, %f)\n", pout, angle);

    D3DXMatrixIdentity(pout);
    pout->u.m[1][1] = cos(angle);
    pout->u.m[2][2] = cos(angle);
    pout->u.m[1][2] = sin(angle);
    pout->u.m[2][1] = -sin(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationY(D3DXMATRIX *pout, FLOAT angle)
{
    TRACE("(%p, %f)\n", pout, angle);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = cos(angle);
    pout->u.m[2][2] = cos(angle);
    pout->u.m[0][2] = -sin(angle);
    pout->u.m[2][0] = sin(angle);
    return pout;
}

D3DXMATRIX * WINAPI D3DXMatrixRotationYawPitchRoll(D3DXMATRIX *out, FLOAT yaw, FLOAT pitch, FLOAT roll)
{
    FLOAT sroll, croll, spitch, cpitch, syaw, cyaw;

    TRACE("out %p, yaw %f, pitch %f, roll %f\n", out, yaw, pitch, roll);

    sroll = sinf(roll);
    croll = cosf(roll);
    spitch = sinf(pitch);
    cpitch = cosf(pitch);
    syaw = sinf(yaw);
    cyaw = cosf(yaw);

    out->u.m[0][0] = sroll * spitch * syaw + croll * cyaw;
    out->u.m[0][1] = sroll * cpitch;
    out->u.m[0][2] = sroll * spitch * cyaw - croll * syaw;
    out->u.m[0][3] = 0.0f;
    out->u.m[1][0] = croll * spitch * syaw - sroll * cyaw;
    out->u.m[1][1] = croll * cpitch;
    out->u.m[1][2] = croll * spitch * cyaw + sroll * syaw;
    out->u.m[1][3] = 0.0f;
    out->u.m[2][0] = cpitch * syaw;
    out->u.m[2][1] = -spitch;
    out->u.m[2][2] = cpitch * cyaw;
    out->u.m[2][3] = 0.0f;
    out->u.m[3][0] = 0.0f;
    out->u.m[3][1] = 0.0f;
    out->u.m[3][2] = 0.0f;
    out->u.m[3][3] = 1.0f;

    return out;
}

D3DXMATRIX* WINAPI D3DXMatrixRotationZ(D3DXMATRIX *pout, FLOAT angle)
{
    TRACE("(%p, %f)\n", pout, angle);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = cos(angle);
    pout->u.m[1][1] = cos(angle);
    pout->u.m[0][1] = sin(angle);
    pout->u.m[1][0] = -sin(angle);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixScaling(D3DXMATRIX *pout, FLOAT sx, FLOAT sy, FLOAT sz)
{
    TRACE("(%p, %f, %f, %f)\n", pout, sx, sy, sz);

    D3DXMatrixIdentity(pout);
    pout->u.m[0][0] = sx;
    pout->u.m[1][1] = sy;
    pout->u.m[2][2] = sz;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixShadow(D3DXMATRIX *pout, CONST D3DXVECTOR4 *plight, CONST D3DXPLANE *pplane)
{
    D3DXPLANE Nplane;
    FLOAT dot;

    TRACE("(%p, %p, %p)\n", pout, plight, pplane);

    D3DXPlaneNormalize(&Nplane, pplane);
    dot = D3DXPlaneDot(&Nplane, plight);
    pout->u.m[0][0] = dot - Nplane.a * plight->x;
    pout->u.m[0][1] = -Nplane.a * plight->y;
    pout->u.m[0][2] = -Nplane.a * plight->z;
    pout->u.m[0][3] = -Nplane.a * plight->w;
    pout->u.m[1][0] = -Nplane.b * plight->x;
    pout->u.m[1][1] = dot - Nplane.b * plight->y;
    pout->u.m[1][2] = -Nplane.b * plight->z;
    pout->u.m[1][3] = -Nplane.b * plight->w;
    pout->u.m[2][0] = -Nplane.c * plight->x;
    pout->u.m[2][1] = -Nplane.c * plight->y;
    pout->u.m[2][2] = dot - Nplane.c * plight->z;
    pout->u.m[2][3] = -Nplane.c * plight->w;
    pout->u.m[3][0] = -Nplane.d * plight->x;
    pout->u.m[3][1] = -Nplane.d * plight->y;
    pout->u.m[3][2] = -Nplane.d * plight->z;
    pout->u.m[3][3] = dot - Nplane.d * plight->w;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixTransformation(D3DXMATRIX *pout, CONST D3DXVECTOR3 *pscalingcenter, CONST D3DXQUATERNION *pscalingrotation, CONST D3DXVECTOR3 *pscaling, CONST D3DXVECTOR3 *protationcenter, CONST D3DXQUATERNION *protation, CONST D3DXVECTOR3 *ptranslation)
{
    D3DXMATRIX m1, m2, m3, m4, m5, m6, m7;
    D3DXQUATERNION prc;
    D3DXVECTOR3 psc, pt;

    TRACE("(%p, %p, %p, %p, %p, %p, %p)\n", pout, pscalingcenter, pscalingrotation, pscaling, protationcenter, protation, ptranslation);

    if ( !pscalingcenter )
    {
        psc.x = 0.0f;
        psc.y = 0.0f;
        psc.z = 0.0f;
    }
    else
    {
        psc.x = pscalingcenter->x;
        psc.y = pscalingcenter->y;
        psc.z = pscalingcenter->z;
    }

    if ( !protationcenter )
    {
        prc.x = 0.0f;
        prc.y = 0.0f;
        prc.z = 0.0f;
    }
    else
    {
        prc.x = protationcenter->x;
        prc.y = protationcenter->y;
        prc.z = protationcenter->z;
    }

    if ( !ptranslation )
    {
        pt.x = 0.0f;
        pt.y = 0.0f;
        pt.z = 0.0f;
    }
    else
    {
        pt.x = ptranslation->x;
        pt.y = ptranslation->y;
        pt.z = ptranslation->z;
    }

    D3DXMatrixTranslation(&m1, -psc.x, -psc.y, -psc.z);

    if ( !pscalingrotation )
    {
        D3DXMatrixIdentity(&m2);
        D3DXMatrixIdentity(&m4);
    }
    else
    {
        D3DXMatrixRotationQuaternion(&m4, pscalingrotation);
        D3DXMatrixInverse(&m2, NULL, &m4);
    }

    if ( !pscaling ) D3DXMatrixIdentity(&m3);
    else D3DXMatrixScaling(&m3, pscaling->x, pscaling->y, pscaling->z);

    if ( !protation ) D3DXMatrixIdentity(&m6);
    else D3DXMatrixRotationQuaternion(&m6, protation);

    D3DXMatrixTranslation(&m5, psc.x - prc.x,  psc.y - prc.y,  psc.z - prc.z);
    D3DXMatrixTranslation(&m7, prc.x + pt.x, prc.y + pt.y, prc.z + pt.z);
    D3DXMatrixMultiply(&m1, &m1, &m2);
    D3DXMatrixMultiply(&m1, &m1, &m3);
    D3DXMatrixMultiply(&m1, &m1, &m4);
    D3DXMatrixMultiply(&m1, &m1, &m5);
    D3DXMatrixMultiply(&m1, &m1, &m6);
    D3DXMatrixMultiply(pout, &m1, &m7);
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixTransformation2D(D3DXMATRIX *pout, CONST D3DXVECTOR2 *pscalingcenter, FLOAT scalingrotation, CONST D3DXVECTOR2 *pscaling, CONST D3DXVECTOR2 *protationcenter, FLOAT rotation, CONST D3DXVECTOR2 *ptranslation)
{
    D3DXQUATERNION rot, sca_rot;
    D3DXVECTOR3 rot_center, sca, sca_center, trans;

    TRACE("(%p, %p, %f, %p, %p, %f, %p)\n", pout, pscalingcenter, scalingrotation, pscaling, protationcenter, rotation, ptranslation);

    if ( pscalingcenter )
    {
        sca_center.x=pscalingcenter->x;
        sca_center.y=pscalingcenter->y;
        sca_center.z=0.0f;
    }
    else
    {
        sca_center.x=0.0f;
        sca_center.y=0.0f;
        sca_center.z=0.0f;
    }

    if ( pscaling )
    {
        sca.x=pscaling->x;
        sca.y=pscaling->y;
        sca.z=1.0f;
    }
    else
    {
        sca.x=1.0f;
        sca.y=1.0f;
        sca.z=1.0f;
    }

    if ( protationcenter )
    {
        rot_center.x=protationcenter->x;
        rot_center.y=protationcenter->y;
        rot_center.z=0.0f;
    }
    else
    {
        rot_center.x=0.0f;
        rot_center.y=0.0f;
        rot_center.z=0.0f;
    }

    if ( ptranslation )
    {
        trans.x=ptranslation->x;
        trans.y=ptranslation->y;
        trans.z=0.0f;
    }
    else
    {
        trans.x=0.0f;
        trans.y=0.0f;
        trans.z=0.0f;
    }

    rot.w=cos(rotation/2.0f);
    rot.x=0.0f;
    rot.y=0.0f;
    rot.z=sin(rotation/2.0f);

    sca_rot.w=cos(scalingrotation/2.0f);
    sca_rot.x=0.0f;
    sca_rot.y=0.0f;
    sca_rot.z=sin(scalingrotation/2.0f);

    D3DXMatrixTransformation(pout, &sca_center, &sca_rot, &sca, &rot_center, &rot, &trans);

    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixTranslation(D3DXMATRIX *pout, FLOAT x, FLOAT y, FLOAT z)
{
    TRACE("(%p, %f, %f, %f)\n", pout, x, y, z);

    D3DXMatrixIdentity(pout);
    pout->u.m[3][0] = x;
    pout->u.m[3][1] = y;
    pout->u.m[3][2] = z;
    return pout;
}

D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX *pout, CONST D3DXMATRIX *pm)
{
    CONST D3DXMATRIX m = *pm;
    int i,j;

    TRACE("(%p, %p)\n", pout, pm);

    for (i=0; i<4; i++)
        for (j=0; j<4; j++) pout->u.m[i][j] = m.u.m[j][i];

    return pout;
}

/*_________________D3DXMatrixStack____________________*/

static const unsigned int INITIAL_STACK_SIZE = 32;

HRESULT WINAPI D3DXCreateMatrixStack(DWORD flags, ID3DXMatrixStack **ppstack)
{
    struct ID3DXMatrixStackImpl *object;

    TRACE("flags %#x, ppstack %p\n", flags, ppstack);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (object == NULL)
    {
        *ppstack = NULL;
        return E_OUTOFMEMORY;
    }
    object->ID3DXMatrixStack_iface.lpVtbl = &ID3DXMatrixStack_Vtbl;
    object->ref = 1;

    object->stack = HeapAlloc(GetProcessHeap(), 0, INITIAL_STACK_SIZE * sizeof(*object->stack));
    if (!object->stack)
    {
        HeapFree(GetProcessHeap(), 0, object);
        *ppstack = NULL;
        return E_OUTOFMEMORY;
    }

    object->current = 0;
    object->stack_size = INITIAL_STACK_SIZE;
    D3DXMatrixIdentity(&object->stack[0]);

    TRACE("Created matrix stack %p\n", object);

    *ppstack = &object->ID3DXMatrixStack_iface;
    return D3D_OK;
}

static inline struct ID3DXMatrixStackImpl *impl_from_ID3DXMatrixStack(ID3DXMatrixStack *iface)
{
  return CONTAINING_RECORD(iface, struct ID3DXMatrixStackImpl, ID3DXMatrixStack_iface);
}

static HRESULT WINAPI ID3DXMatrixStackImpl_QueryInterface(ID3DXMatrixStack *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXMatrixStack)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3DXMatrixStack_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXMatrixStackImpl_AddRef(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) : AddRef from %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI ID3DXMatrixStackImpl_Release(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This->stack);
        HeapFree(GetProcessHeap(), 0, This);
    }
    TRACE("(%p) : ReleaseRef to %d\n", This, ref);
    return ref;
}

static D3DXMATRIX* WINAPI ID3DXMatrixStackImpl_GetTop(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    return &This->stack[This->current];
}

static HRESULT WINAPI ID3DXMatrixStackImpl_LoadIdentity(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixIdentity(&This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_LoadMatrix(ID3DXMatrixStack *iface, CONST D3DXMATRIX *pm)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    This->stack[This->current] = *pm;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_MultMatrix(ID3DXMatrixStack *iface, CONST D3DXMATRIX *pm)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], pm);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_MultMatrixLocal(ID3DXMatrixStack *iface, CONST D3DXMATRIX *pm)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixMultiply(&This->stack[This->current], pm, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Pop(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    /* Popping the last element on the stack returns D3D_OK, but does nothing. */
    if (!This->current) return D3D_OK;

    if (This->current <= This->stack_size / 4 && This->stack_size >= INITIAL_STACK_SIZE * 2)
    {
        unsigned int new_size;
        D3DXMATRIX *new_stack;

        new_size = This->stack_size / 2;
        new_stack = HeapReAlloc(GetProcessHeap(), 0, This->stack, new_size * sizeof(*new_stack));
        if (new_stack)
        {
            This->stack_size = new_size;
            This->stack = new_stack;
        }
    }

    --This->current;

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Push(ID3DXMatrixStack *iface)
{
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    if (This->current == This->stack_size - 1)
    {
        unsigned int new_size;
        D3DXMATRIX *new_stack;

        if (This->stack_size > UINT_MAX / 2) return E_OUTOFMEMORY;

        new_size = This->stack_size * 2;
        new_stack = HeapReAlloc(GetProcessHeap(), 0, This->stack, new_size * sizeof(*new_stack));
        if (!new_stack) return E_OUTOFMEMORY;

        This->stack_size = new_size;
        This->stack = new_stack;
    }

    ++This->current;
    This->stack[This->current] = This->stack[This->current - 1];

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateAxis(ID3DXMatrixStack *iface, CONST D3DXVECTOR3 *pv, FLOAT angle)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixRotationAxis(&temp, pv, angle);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateAxisLocal(ID3DXMatrixStack *iface, CONST D3DXVECTOR3 *pv, FLOAT angle)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixRotationAxis(&temp, pv, angle);
    D3DXMatrixMultiply(&This->stack[This->current], &temp, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateYawPitchRoll(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixRotationYawPitchRoll(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_RotateYawPitchRollLocal(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixRotationYawPitchRoll(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &temp, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Scale(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixScaling(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_ScaleLocal(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixScaling(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &temp, &This->stack[This->current]);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_Translate(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixTranslation(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &This->stack[This->current], &temp);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXMatrixStackImpl_TranslateLocal(ID3DXMatrixStack *iface, FLOAT x, FLOAT y, FLOAT z)
{
    D3DXMATRIX temp;
    struct ID3DXMatrixStackImpl *This = impl_from_ID3DXMatrixStack(iface);

    TRACE("iface %p\n", iface);

    D3DXMatrixTranslation(&temp, x, y, z);
    D3DXMatrixMultiply(&This->stack[This->current], &temp,&This->stack[This->current]);

    return D3D_OK;
}

static const ID3DXMatrixStackVtbl ID3DXMatrixStack_Vtbl =
{
    ID3DXMatrixStackImpl_QueryInterface,
    ID3DXMatrixStackImpl_AddRef,
    ID3DXMatrixStackImpl_Release,
    ID3DXMatrixStackImpl_Pop,
    ID3DXMatrixStackImpl_Push,
    ID3DXMatrixStackImpl_LoadIdentity,
    ID3DXMatrixStackImpl_LoadMatrix,
    ID3DXMatrixStackImpl_MultMatrix,
    ID3DXMatrixStackImpl_MultMatrixLocal,
    ID3DXMatrixStackImpl_RotateAxis,
    ID3DXMatrixStackImpl_RotateAxisLocal,
    ID3DXMatrixStackImpl_RotateYawPitchRoll,
    ID3DXMatrixStackImpl_RotateYawPitchRollLocal,
    ID3DXMatrixStackImpl_Scale,
    ID3DXMatrixStackImpl_ScaleLocal,
    ID3DXMatrixStackImpl_Translate,
    ID3DXMatrixStackImpl_TranslateLocal,
    ID3DXMatrixStackImpl_GetTop
};

/*_________________D3DXPLANE________________*/

D3DXPLANE* WINAPI D3DXPlaneFromPointNormal(D3DXPLANE *pout, CONST D3DXVECTOR3 *pvpoint, CONST D3DXVECTOR3 *pvnormal)
{
    TRACE("(%p, %p, %p)\n", pout, pvpoint, pvnormal);

    pout->a = pvnormal->x;
    pout->b = pvnormal->y;
    pout->c = pvnormal->z;
    pout->d = -D3DXVec3Dot(pvpoint, pvnormal);
    return pout;
}

D3DXPLANE* WINAPI D3DXPlaneFromPoints(D3DXPLANE *pout, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pv3)
{
    D3DXVECTOR3 edge1, edge2, normal, Nnormal;

    TRACE("(%p, %p, %p, %p)\n", pout, pv1, pv2, pv3);

    edge1.x = 0.0f; edge1.y = 0.0f; edge1.z = 0.0f;
    edge2.x = 0.0f; edge2.y = 0.0f; edge2.z = 0.0f;
    D3DXVec3Subtract(&edge1, pv2, pv1);
    D3DXVec3Subtract(&edge2, pv3, pv1);
    D3DXVec3Cross(&normal, &edge1, &edge2);
    D3DXVec3Normalize(&Nnormal, &normal);
    D3DXPlaneFromPointNormal(pout, pv1, &Nnormal);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXPlaneIntersectLine(D3DXVECTOR3 *pout, CONST D3DXPLANE *pp, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2)
{
    D3DXVECTOR3 direction, normal;
    FLOAT dot, temp;

    TRACE("(%p, %p, %p, %p)\n", pout, pp, pv1, pv2);

    normal.x = pp->a;
    normal.y = pp->b;
    normal.z = pp->c;
    direction.x = pv2->x - pv1->x;
    direction.y = pv2->y - pv1->y;
    direction.z = pv2->z - pv1->z;
    dot = D3DXVec3Dot(&normal, &direction);
    if ( !dot ) return NULL;
    temp = ( pp->d + D3DXVec3Dot(&normal, pv1) ) / dot;
    pout->x = pv1->x - temp * direction.x;
    pout->y = pv1->y - temp * direction.y;
    pout->z = pv1->z - temp * direction.z;
    return pout;
}

D3DXPLANE * WINAPI D3DXPlaneNormalize(D3DXPLANE *out, const D3DXPLANE *p)
{
    FLOAT norm;

    TRACE("out %p, p %p\n", out, p);

    norm = sqrtf(p->a * p->a + p->b * p->b + p->c * p->c);
    if (norm)
    {
        out->a = p->a / norm;
        out->b = p->b / norm;
        out->c = p->c / norm;
        out->d = p->d / norm;
    }
    else
    {
        out->a = 0.0f;
        out->b = 0.0f;
        out->c = 0.0f;
        out->d = 0.0f;
    }

    return out;
}

D3DXPLANE* WINAPI D3DXPlaneTransform(D3DXPLANE *pout, CONST D3DXPLANE *pplane, CONST D3DXMATRIX *pm)
{
    CONST D3DXPLANE plane = *pplane;

    TRACE("(%p, %p, %p)\n", pout, pplane, pm);

    pout->a = pm->u.m[0][0] * plane.a + pm->u.m[1][0] * plane.b + pm->u.m[2][0] * plane.c + pm->u.m[3][0] * plane.d;
    pout->b = pm->u.m[0][1] * plane.a + pm->u.m[1][1] * plane.b + pm->u.m[2][1] * plane.c + pm->u.m[3][1] * plane.d;
    pout->c = pm->u.m[0][2] * plane.a + pm->u.m[1][2] * plane.b + pm->u.m[2][2] * plane.c + pm->u.m[3][2] * plane.d;
    pout->d = pm->u.m[0][3] * plane.a + pm->u.m[1][3] * plane.b + pm->u.m[2][3] * plane.c + pm->u.m[3][3] * plane.d;
    return pout;
}

D3DXPLANE* WINAPI D3DXPlaneTransformArray(D3DXPLANE* out, UINT outstride, CONST D3DXPLANE* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXPlaneTransform(
            (D3DXPLANE*)((char*)out + outstride * i),
            (CONST D3DXPLANE*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*_________________D3DXQUATERNION________________*/

D3DXQUATERNION* WINAPI D3DXQuaternionBaryCentric(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq1, CONST D3DXQUATERNION *pq2, CONST D3DXQUATERNION *pq3, FLOAT f, FLOAT g)
{
    D3DXQUATERNION temp1, temp2;

    TRACE("(%p, %p, %p, %p, %f, %f)\n", pout, pq1, pq2, pq3, f, g);

    D3DXQuaternionSlerp(pout, D3DXQuaternionSlerp(&temp1, pq1, pq2, f + g), D3DXQuaternionSlerp(&temp2, pq1, pq3, f+g), g / (f + g));
    return pout;
}

D3DXQUATERNION * WINAPI D3DXQuaternionExp(D3DXQUATERNION *out, const D3DXQUATERNION *q)
{
    FLOAT norm;

    TRACE("out %p, q %p\n", out, q);

    norm = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z);
    if (norm)
    {
        out->x = sinf(norm) * q->x / norm;
        out->y = sinf(norm) * q->y / norm;
        out->z = sinf(norm) * q->z / norm;
        out->w = cosf(norm);
    }
    else
    {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        out->w = 1.0f;
    }

    return out;
}

D3DXQUATERNION* WINAPI D3DXQuaternionInverse(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq)
{
    D3DXQUATERNION out;
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pq);

    norm = D3DXQuaternionLengthSq(pq);

    out.x = -pq->x / norm;
    out.y = -pq->y / norm;
    out.z = -pq->z / norm;
    out.w = pq->w / norm;

    *pout =out;
    return pout;
}

D3DXQUATERNION * WINAPI D3DXQuaternionLn(D3DXQUATERNION *out, const D3DXQUATERNION *q)
{
    FLOAT t;

    TRACE("out %p, q %p\n", out, q);

    if ((q->w >= 1.0f) || (q->w == -1.0f))
        t = 1.0f;
    else
        t = acosf(q->w) / sqrtf(1.0f - q->w * q->w);

    out->x = t * q->x;
    out->y = t * q->y;
    out->z = t * q->z;
    out->w = 0.0f;

    return out;
}

D3DXQUATERNION* WINAPI D3DXQuaternionMultiply(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq1, CONST D3DXQUATERNION *pq2)
{
    D3DXQUATERNION out;

    TRACE("(%p, %p, %p)\n", pout, pq1, pq2);

    out.x = pq2->w * pq1->x + pq2->x * pq1->w + pq2->y * pq1->z - pq2->z * pq1->y;
    out.y = pq2->w * pq1->y - pq2->x * pq1->z + pq2->y * pq1->w + pq2->z * pq1->x;
    out.z = pq2->w * pq1->z + pq2->x * pq1->y - pq2->y * pq1->x + pq2->z * pq1->w;
    out.w = pq2->w * pq1->w - pq2->x * pq1->x - pq2->y * pq1->y - pq2->z * pq1->z;
    *pout = out;
    return pout;
}

D3DXQUATERNION * WINAPI D3DXQuaternionNormalize(D3DXQUATERNION *out, const D3DXQUATERNION *q)
{
    FLOAT norm;

    TRACE("out %p, q %p\n", out, q);

    norm = D3DXQuaternionLength(q);

    out->x = q->x / norm;
    out->y = q->y / norm;
    out->z = q->z / norm;
    out->w = q->w / norm;

    return out;
}

D3DXQUATERNION * WINAPI D3DXQuaternionRotationAxis(D3DXQUATERNION *out, const D3DXVECTOR3 *v, FLOAT angle)
{
    D3DXVECTOR3 temp;

    TRACE("out %p, v %p, angle %f\n", out, v, angle);

    D3DXVec3Normalize(&temp, v);

    out->x = sinf(angle / 2.0f) * temp.x;
    out->y = sinf(angle / 2.0f) * temp.y;
    out->z = sinf(angle / 2.0f) * temp.z;
    out->w = cosf(angle / 2.0f);

    return out;
}

D3DXQUATERNION * WINAPI D3DXQuaternionRotationMatrix(D3DXQUATERNION *out, const D3DXMATRIX *m)
{
    FLOAT s, trace;

    TRACE("out %p, m %p\n", out, m);

    trace = m->u.m[0][0] + m->u.m[1][1] + m->u.m[2][2] + 1.0f;
    if (trace > 1.0f)
    {
        s = 2.0f * sqrtf(trace);
        out->x = (m->u.m[1][2] - m->u.m[2][1]) / s;
        out->y = (m->u.m[2][0] - m->u.m[0][2]) / s;
        out->z = (m->u.m[0][1] - m->u.m[1][0]) / s;
        out->w = 0.25f * s;
    }
    else
    {
        int i, maxi = 0;

        for (i = 1; i < 3; i++)
        {
            if (m->u.m[i][i] > m->u.m[maxi][maxi])
                maxi = i;
        }

        switch (maxi)
        {
            case 0:
                s = 2.0f * sqrtf(1.0f + m->u.m[0][0] - m->u.m[1][1] - m->u.m[2][2]);
                out->x = 0.25f * s;
                out->y = (m->u.m[0][1] + m->u.m[1][0]) / s;
                out->z = (m->u.m[0][2] + m->u.m[2][0]) / s;
                out->w = (m->u.m[1][2] - m->u.m[2][1]) / s;
                break;

            case 1:
                s = 2.0f * sqrtf(1.0f + m->u.m[1][1] - m->u.m[0][0] - m->u.m[2][2]);
                out->x = (m->u.m[0][1] + m->u.m[1][0]) / s;
                out->y = 0.25f * s;
                out->z = (m->u.m[1][2] + m->u.m[2][1]) / s;
                out->w = (m->u.m[2][0] - m->u.m[0][2]) / s;
                break;

            case 2:
                s = 2.0f * sqrtf(1.0f + m->u.m[2][2] - m->u.m[0][0] - m->u.m[1][1]);
                out->x = (m->u.m[0][2] + m->u.m[2][0]) / s;
                out->y = (m->u.m[1][2] + m->u.m[2][1]) / s;
                out->z = 0.25f * s;
                out->w = (m->u.m[0][1] - m->u.m[1][0]) / s;
                break;
        }
    }

    return out;
}

D3DXQUATERNION * WINAPI D3DXQuaternionRotationYawPitchRoll(D3DXQUATERNION *out, FLOAT yaw, FLOAT pitch, FLOAT roll)
{
    FLOAT syaw, cyaw, spitch, cpitch, sroll, croll;

    TRACE("out %p, yaw %f, pitch %f, roll %f\n", out, yaw, pitch, roll);

    syaw = sinf(yaw / 2.0f);
    cyaw = cosf(yaw / 2.0f);
    spitch = sinf(pitch / 2.0f);
    cpitch = cosf(pitch / 2.0f);
    sroll = sinf(roll / 2.0f);
    croll = cosf(roll / 2.0f);

    out->x = syaw * cpitch * sroll + cyaw * spitch * croll;
    out->y = syaw * cpitch * croll - cyaw * spitch * sroll;
    out->z = cyaw * cpitch * sroll - syaw * spitch * croll;
    out->w = cyaw * cpitch * croll + syaw * spitch * sroll;

    return out;
}

D3DXQUATERNION * WINAPI D3DXQuaternionSlerp(D3DXQUATERNION *out, const D3DXQUATERNION *q1,
        const D3DXQUATERNION *q2, FLOAT t)
{
    FLOAT dot, temp;

    TRACE("out %p, q1 %p, q2 %p, t %f\n", out, q1, q2, t);

    temp = 1.0f - t;
    dot = D3DXQuaternionDot(q1, q2);
    if (dot < 0.0f)
    {
        t = -t;
        dot = -dot;
    }

    if (1.0f - dot > 0.001f)
    {
        FLOAT theta = acosf(dot);

        temp = sinf(theta * temp) / sinf(theta);
        t = sinf(theta * t) / sinf(theta);
    }

    out->x = temp * q1->x + t * q2->x;
    out->y = temp * q1->y + t * q2->y;
    out->z = temp * q1->z + t * q2->z;
    out->w = temp * q1->w + t * q2->w;

    return out;
}

D3DXQUATERNION* WINAPI D3DXQuaternionSquad(D3DXQUATERNION *pout, CONST D3DXQUATERNION *pq1, CONST D3DXQUATERNION *pq2, CONST D3DXQUATERNION *pq3, CONST D3DXQUATERNION *pq4, FLOAT t)
{
    D3DXQUATERNION temp1, temp2;

    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pq1, pq2, pq3, pq4, t);

    D3DXQuaternionSlerp(pout, D3DXQuaternionSlerp(&temp1, pq1, pq4, t), D3DXQuaternionSlerp(&temp2, pq2, pq3, t), 2.0f * t * (1.0f - t));
    return pout;
}

static D3DXQUATERNION add_diff(CONST D3DXQUATERNION *q1, CONST D3DXQUATERNION *q2, CONST FLOAT add)
{
    D3DXQUATERNION temp;

    temp.x = q1->x + add * q2->x;
    temp.y = q1->y + add * q2->y;
    temp.z = q1->z + add * q2->z;
    temp.w = q1->w + add * q2->w;

    return temp;
}

void WINAPI D3DXQuaternionSquadSetup(D3DXQUATERNION *paout, D3DXQUATERNION *pbout, D3DXQUATERNION *pcout, CONST D3DXQUATERNION *pq0, CONST D3DXQUATERNION *pq1, CONST D3DXQUATERNION *pq2, CONST D3DXQUATERNION *pq3)
{
    D3DXQUATERNION q, temp1, temp2, temp3, zero;

    TRACE("(%p, %p, %p, %p, %p, %p, %p)\n", paout, pbout, pcout, pq0, pq1, pq2, pq3);

    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    zero.w = 0.0f;

    if ( D3DXQuaternionDot(pq0, pq1) <  0.0f )
        temp2 = add_diff(&zero, pq0, -1.0f);
    else
        temp2 = *pq0;

    if ( D3DXQuaternionDot(pq1, pq2) < 0.0f )
        *pcout = add_diff(&zero, pq2, -1.0f);
    else
        *pcout = *pq2;

    if ( D3DXQuaternionDot(pcout, pq3) < 0.0f )
        temp3 = add_diff(&zero, pq3, -1.0f);
    else
        temp3 = *pq3;

    D3DXQuaternionInverse(&temp1, pq1);
    D3DXQuaternionMultiply(&temp2, &temp1, &temp2);
    D3DXQuaternionLn(&temp2, &temp2);
    D3DXQuaternionMultiply(&q, &temp1, pcout);
    D3DXQuaternionLn(&q, &q);
    temp1 = add_diff(&temp2, &q, 1.0f);
    temp1.x *= -0.25f;
    temp1.y *= -0.25f;
    temp1.z *= -0.25f;
    temp1.w *= -0.25f;
    D3DXQuaternionExp(&temp1, &temp1);
    D3DXQuaternionMultiply(paout, pq1, &temp1);

    D3DXQuaternionInverse(&temp1, pcout);
    D3DXQuaternionMultiply(&temp2, &temp1, pq1);
    D3DXQuaternionLn(&temp2, &temp2);
    D3DXQuaternionMultiply(&q, &temp1, &temp3);
    D3DXQuaternionLn(&q, &q);
    temp1 = add_diff(&temp2, &q, 1.0f);
    temp1.x *= -0.25f;
    temp1.y *= -0.25f;
    temp1.z *= -0.25f;
    temp1.w *= -0.25f;
    D3DXQuaternionExp(&temp1, &temp1);
    D3DXQuaternionMultiply(pbout, pcout, &temp1);

    return;
}

void WINAPI D3DXQuaternionToAxisAngle(CONST D3DXQUATERNION *pq, D3DXVECTOR3 *paxis, FLOAT *pangle)
{
    TRACE("(%p, %p, %p)\n", pq, paxis, pangle);

    paxis->x = pq->x;
    paxis->y = pq->y;
    paxis->z = pq->z;
    *pangle = 2.0f * acos(pq->w);
}

/*_________________D3DXVec2_____________________*/

D3DXVECTOR2* WINAPI D3DXVec2BaryCentric(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pv2, CONST D3DXVECTOR2 *pv3, FLOAT f, FLOAT g)
{
    TRACE("(%p, %p, %p, %p, %f, %f)\n", pout, pv1, pv2, pv3, f, g);

    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2CatmullRom(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv0, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pv2, CONST D3DXVECTOR2 *pv3, FLOAT s)
{
    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv0, pv1, pv2, pv3, s);

    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2Hermite(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv1, CONST D3DXVECTOR2 *pt1, CONST D3DXVECTOR2 *pv2, CONST D3DXVECTOR2 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv1, pt1, pv2, pt2, s);

    h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
    h2 = s * s * s - 2.0f * s * s + s;
    h3 = -2.0f * s * s * s + 3.0f * s * s;
    h4 = s * s * s - s * s;

    pout->x = h1 * (pv1->x) + h2 * (pt1->x) + h3 * (pv2->x) + h4 * (pt2->x);
    pout->y = h1 * (pv1->y) + h2 * (pt1->y) + h3 * (pv2->y) + h4 * (pt2->y);
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2Normalize(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv)
{
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pv);

    norm = D3DXVec2Length(pv);
    if ( !norm )
    {
        pout->x = 0.0f;
        pout->y = 0.0f;
    }
    else
    {
        pout->x = pv->x / norm;
        pout->y = pv->y / norm;
    }

    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec2Transform(D3DXVECTOR4 *pout, CONST D3DXVECTOR2 *pv, CONST D3DXMATRIX *pm)
{
    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    pout->x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y  + pm->u.m[3][0];
    pout->y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y  + pm->u.m[3][1];
    pout->z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y  + pm->u.m[3][2];
    pout->w = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y  + pm->u.m[3][3];
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec2TransformArray(D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR2* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec2Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformCoord(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv, CONST D3DXMATRIX *pm)
{
    D3DXVECTOR2 v;
    FLOAT norm;

    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    v = *pv;
    norm = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[3][3];

    pout->x = (pm->u.m[0][0] * v.x + pm->u.m[1][0] * v.y + pm->u.m[3][0]) / norm;
    pout->y = (pm->u.m[0][1] * v.x + pm->u.m[1][1] * v.y + pm->u.m[3][1]) / norm;

    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformCoordArray(D3DXVECTOR2* out, UINT outstride, CONST D3DXVECTOR2* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec2TransformCoord(
            (D3DXVECTOR2*)((char*)out + outstride * i),
            (CONST D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformNormal(D3DXVECTOR2 *pout, CONST D3DXVECTOR2 *pv, CONST D3DXMATRIX *pm)
{
    CONST D3DXVECTOR2 v = *pv;
    pout->x = pm->u.m[0][0] * v.x + pm->u.m[1][0] * v.y;
    pout->y = pm->u.m[0][1] * v.x + pm->u.m[1][1] * v.y;
    return pout;
}

D3DXVECTOR2* WINAPI D3DXVec2TransformNormalArray(D3DXVECTOR2* out, UINT outstride, CONST D3DXVECTOR2 *in, UINT instride, CONST D3DXMATRIX *matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec2TransformNormal(
            (D3DXVECTOR2*)((char*)out + outstride * i),
            (CONST D3DXVECTOR2*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

/*_________________D3DXVec3_____________________*/

D3DXVECTOR3* WINAPI D3DXVec3BaryCentric(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pv3, FLOAT f, FLOAT g)
{
    TRACE("(%p, %p, %p, %p, %f, %f)\n", pout, pv1, pv2, pv3, f, g);

    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3CatmullRom( D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv0, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pv3, FLOAT s)
{
    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv0, pv1, pv2, pv3, s);

    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    pout->z = 0.5f * (2.0f * pv1->z + (pv2->z - pv0->z) *s + (2.0f *pv0->z - 5.0f * pv1->z + 4.0f * pv2->z - pv3->z) * s * s + (pv3->z -3.0f * pv2->z + 3.0f * pv1->z - pv0->z) * s * s * s);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Hermite(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv1, CONST D3DXVECTOR3 *pt1, CONST D3DXVECTOR3 *pv2, CONST D3DXVECTOR3 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv1, pt1, pv2, pt2, s);

    h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
    h2 = s * s * s - 2.0f * s * s + s;
    h3 = -2.0f * s * s * s + 3.0f * s * s;
    h4 = s * s * s - s * s;

    pout->x = h1 * (pv1->x) + h2 * (pt1->x) + h3 * (pv2->x) + h4 * (pt2->x);
    pout->y = h1 * (pv1->y) + h2 * (pt1->y) + h3 * (pv2->y) + h4 * (pt2->y);
    pout->z = h1 * (pv1->z) + h2 * (pt1->z) + h3 * (pv2->z) + h4 * (pt2->z);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv)
{
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pv);

    norm = D3DXVec3Length(pv);
    if ( !norm )
    {
        pout->x = 0.0f;
        pout->y = 0.0f;
        pout->z = 0.0f;
    }
    else
    {
        pout->x = pv->x / norm;
        pout->y = pv->y / norm;
        pout->z = pv->z / norm;
    }

    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3Project(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DVIEWPORT9 *pviewport, CONST D3DXMATRIX *pprojection, CONST D3DXMATRIX *pview, CONST D3DXMATRIX *pworld)
{
    D3DXMATRIX m;

    TRACE("(%p, %p, %p, %p, %p, %p)\n", pout, pv, pviewport, pprojection, pview, pworld);

    D3DXMatrixIdentity(&m);
    if (pworld) D3DXMatrixMultiply(&m, &m, pworld);
    if (pview) D3DXMatrixMultiply(&m, &m, pview);
    if (pprojection) D3DXMatrixMultiply(&m, &m, pprojection);

    D3DXVec3TransformCoord(pout, pv, &m);

    if (pviewport)
    {
        pout->x = pviewport->X +  ( 1.0f + pout->x ) * pviewport->Width / 2.0f;
        pout->y = pviewport->Y +  ( 1.0f - pout->y ) * pviewport->Height / 2.0f;
        pout->z = pviewport->MinZ + pout->z * ( pviewport->MaxZ - pviewport->MinZ );
    }
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3ProjectArray(D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DVIEWPORT9* viewport, CONST D3DXMATRIX* projection, CONST D3DXMATRIX* view, CONST D3DXMATRIX* world, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %p, %p, %p, %u)\n", out, outstride, in, instride, viewport, projection, view, world, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3Project(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            viewport, projection, view, world);
    }
    return out;
}

D3DXVECTOR4* WINAPI D3DXVec3Transform(D3DXVECTOR4 *pout, CONST D3DXVECTOR3 *pv, CONST D3DXMATRIX *pm)
{
    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    pout->x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z + pm->u.m[3][0];
    pout->y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z + pm->u.m[3][1];
    pout->z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z + pm->u.m[3][2];
    pout->w = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[2][3] * pv->z + pm->u.m[3][3];
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec3TransformArray(D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoord(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DXMATRIX *pm)
{
    D3DXVECTOR3 out;
    FLOAT norm;

    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    norm = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[2][3] *pv->z + pm->u.m[3][3];

    out.x = (pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z + pm->u.m[3][0]) / norm;
    out.y = (pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z + pm->u.m[3][1]) / norm;
    out.z = (pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z + pm->u.m[3][2]) / norm;

    *pout = out;

    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformCoordArray(D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3TransformCoord(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR3* WINAPI D3DXVec3TransformNormal(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DXMATRIX *pm)
{
    CONST D3DXVECTOR3 v = *pv;

    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    pout->x = pm->u.m[0][0] * v.x + pm->u.m[1][0] * v.y + pm->u.m[2][0] * v.z;
    pout->y = pm->u.m[0][1] * v.x + pm->u.m[1][1] * v.y + pm->u.m[2][1] * v.z;
    pout->z = pm->u.m[0][2] * v.x + pm->u.m[1][2] * v.y + pm->u.m[2][2] * v.z;
    return pout;

}

D3DXVECTOR3* WINAPI D3DXVec3TransformNormalArray(D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3TransformNormal(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

D3DXVECTOR3* WINAPI D3DXVec3Unproject(D3DXVECTOR3 *pout, CONST D3DXVECTOR3 *pv, CONST D3DVIEWPORT9 *pviewport, CONST D3DXMATRIX *pprojection, CONST D3DXMATRIX *pview, CONST D3DXMATRIX *pworld)
{
    D3DXMATRIX m;

    TRACE("(%p, %p, %p, %p, %p, %p)\n", pout, pv, pviewport, pprojection, pview, pworld);

    D3DXMatrixIdentity(&m);
    if (pworld) D3DXMatrixMultiply(&m, &m, pworld);
    if (pview) D3DXMatrixMultiply(&m, &m, pview);
    if (pprojection) D3DXMatrixMultiply(&m, &m, pprojection);
    D3DXMatrixInverse(&m, NULL, &m);

    *pout = *pv;
    if (pviewport)
    {
        pout->x = 2.0f * ( pout->x - pviewport->X ) / pviewport->Width - 1.0f;
        pout->y = 1.0f - 2.0f * ( pout->y - pviewport->Y ) / pviewport->Height;
        pout->z = ( pout->z - pviewport->MinZ) / ( pviewport->MaxZ - pviewport->MinZ );
    }
    D3DXVec3TransformCoord(pout, pout, &m);
    return pout;
}

D3DXVECTOR3* WINAPI D3DXVec3UnprojectArray(D3DXVECTOR3* out, UINT outstride, CONST D3DXVECTOR3* in, UINT instride, CONST D3DVIEWPORT9* viewport, CONST D3DXMATRIX* projection, CONST D3DXMATRIX* view, CONST D3DXMATRIX* world, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %p, %p, %p, %u)\n", out, outstride, in, instride, viewport, projection, view, world, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec3Unproject(
            (D3DXVECTOR3*)((char*)out + outstride * i),
            (CONST D3DXVECTOR3*)((const char*)in + instride * i),
            viewport, projection, view, world);
    }
    return out;
}

/*_________________D3DXVec4_____________________*/

D3DXVECTOR4* WINAPI D3DXVec4BaryCentric(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3, FLOAT f, FLOAT g)
{
    TRACE("(%p, %p, %p, %p, %f, %f)\n", pout, pv1, pv2, pv3, f, g);

    pout->x = (1.0f-f-g) * (pv1->x) + f * (pv2->x) + g * (pv3->x);
    pout->y = (1.0f-f-g) * (pv1->y) + f * (pv2->y) + g * (pv3->y);
    pout->z = (1.0f-f-g) * (pv1->z) + f * (pv2->z) + g * (pv3->z);
    pout->w = (1.0f-f-g) * (pv1->w) + f * (pv2->w) + g * (pv3->w);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4CatmullRom(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv0, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3, FLOAT s)
{
    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv0, pv1, pv2, pv3, s);

    pout->x = 0.5f * (2.0f * pv1->x + (pv2->x - pv0->x) *s + (2.0f *pv0->x - 5.0f * pv1->x + 4.0f * pv2->x - pv3->x) * s * s + (pv3->x -3.0f * pv2->x + 3.0f * pv1->x - pv0->x) * s * s * s);
    pout->y = 0.5f * (2.0f * pv1->y + (pv2->y - pv0->y) *s + (2.0f *pv0->y - 5.0f * pv1->y + 4.0f * pv2->y - pv3->y) * s * s + (pv3->y -3.0f * pv2->y + 3.0f * pv1->y - pv0->y) * s * s * s);
    pout->z = 0.5f * (2.0f * pv1->z + (pv2->z - pv0->z) *s + (2.0f *pv0->z - 5.0f * pv1->z + 4.0f * pv2->z - pv3->z) * s * s + (pv3->z -3.0f * pv2->z + 3.0f * pv1->z - pv0->z) * s * s * s);
    pout->w = 0.5f * (2.0f * pv1->w + (pv2->w - pv0->w) *s + (2.0f *pv0->w - 5.0f * pv1->w + 4.0f * pv2->w - pv3->w) * s * s + (pv3->w -3.0f * pv2->w + 3.0f * pv1->w - pv0->w) * s * s * s);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Cross(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pv3)
{
    D3DXVECTOR4 out;

    TRACE("(%p, %p, %p, %p)\n", pout, pv1, pv2, pv3);

    out.x = pv1->y * (pv2->z * pv3->w - pv3->z * pv2->w) - pv1->z * (pv2->y * pv3->w - pv3->y * pv2->w) + pv1->w * (pv2->y * pv3->z - pv2->z *pv3->y);
    out.y = -(pv1->x * (pv2->z * pv3->w - pv3->z * pv2->w) - pv1->z * (pv2->x * pv3->w - pv3->x * pv2->w) + pv1->w * (pv2->x * pv3->z - pv3->x * pv2->z));
    out.z = pv1->x * (pv2->y * pv3->w - pv3->y * pv2->w) - pv1->y * (pv2->x *pv3->w - pv3->x * pv2->w) + pv1->w * (pv2->x * pv3->y - pv3->x * pv2->y);
    out.w = -(pv1->x * (pv2->y * pv3->z - pv3->y * pv2->z) - pv1->y * (pv2->x * pv3->z - pv3->x *pv2->z) + pv1->z * (pv2->x * pv3->y - pv3->x * pv2->y));
    *pout = out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Hermite(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv1, CONST D3DXVECTOR4 *pt1, CONST D3DXVECTOR4 *pv2, CONST D3DXVECTOR4 *pt2, FLOAT s)
{
    FLOAT h1, h2, h3, h4;

    TRACE("(%p, %p, %p, %p, %p, %f)\n", pout, pv1, pt1, pv2, pt2, s);

    h1 = 2.0f * s * s * s - 3.0f * s * s + 1.0f;
    h2 = s * s * s - 2.0f * s * s + s;
    h3 = -2.0f * s * s * s + 3.0f * s * s;
    h4 = s * s * s - s * s;

    pout->x = h1 * (pv1->x) + h2 * (pt1->x) + h3 * (pv2->x) + h4 * (pt2->x);
    pout->y = h1 * (pv1->y) + h2 * (pt1->y) + h3 * (pv2->y) + h4 * (pt2->y);
    pout->z = h1 * (pv1->z) + h2 * (pt1->z) + h3 * (pv2->z) + h4 * (pt2->z);
    pout->w = h1 * (pv1->w) + h2 * (pt1->w) + h3 * (pv2->w) + h4 * (pt2->w);
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Normalize(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv)
{
    FLOAT norm;

    TRACE("(%p, %p)\n", pout, pv);

    norm = D3DXVec4Length(pv);

    pout->x = pv->x / norm;
    pout->y = pv->y / norm;
    pout->z = pv->z / norm;
    pout->w = pv->w / norm;

    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4Transform(D3DXVECTOR4 *pout, CONST D3DXVECTOR4 *pv, CONST D3DXMATRIX *pm)
{
    D3DXVECTOR4 out;

    TRACE("(%p, %p, %p)\n", pout, pv, pm);

    out.x = pm->u.m[0][0] * pv->x + pm->u.m[1][0] * pv->y + pm->u.m[2][0] * pv->z + pm->u.m[3][0] * pv->w;
    out.y = pm->u.m[0][1] * pv->x + pm->u.m[1][1] * pv->y + pm->u.m[2][1] * pv->z + pm->u.m[3][1] * pv->w;
    out.z = pm->u.m[0][2] * pv->x + pm->u.m[1][2] * pv->y + pm->u.m[2][2] * pv->z + pm->u.m[3][2] * pv->w;
    out.w = pm->u.m[0][3] * pv->x + pm->u.m[1][3] * pv->y + pm->u.m[2][3] * pv->z + pm->u.m[3][3] * pv->w;
    *pout = out;
    return pout;
}

D3DXVECTOR4* WINAPI D3DXVec4TransformArray(D3DXVECTOR4* out, UINT outstride, CONST D3DXVECTOR4* in, UINT instride, CONST D3DXMATRIX* matrix, UINT elements)
{
    UINT i;

    TRACE("(%p, %u, %p, %u, %p, %u)\n", out, outstride, in, instride, matrix, elements);

    for (i = 0; i < elements; ++i) {
        D3DXVec4Transform(
            (D3DXVECTOR4*)((char*)out + outstride * i),
            (CONST D3DXVECTOR4*)((const char*)in + instride * i),
            matrix);
    }
    return out;
}

unsigned short float_32_to_16(const float in)
{
    int exp = 0, origexp;
    float tmp = fabs(in);
    int sign = (copysignf(1, in) < 0);
    unsigned int mantissa;
    unsigned short ret;

    /* Deal with special numbers */
    if (isinf(in)) return (sign ? 0xffff : 0x7fff);
    if (isnan(in)) return (sign ? 0xffff : 0x7fff);
    if (in == 0.0f) return (sign ? 0x8000 : 0x0000);

    if (tmp < powf(2, 10))
    {
        do
        {
            tmp *= 2.0f;
            exp--;
        } while (tmp < powf(2, 10));
    }
    else if (tmp >= powf(2, 11))
    {
        do
        {
            tmp /= 2.0f;
            exp++;
        } while (tmp >= powf(2, 11));
    }

    exp += 10;  /* Normalize the mantissa */
    exp += 15;  /* Exponent is encoded with excess 15 */

    origexp = exp;

    mantissa = (unsigned int) tmp;
    if ((tmp - mantissa == 0.5f && mantissa % 2 == 1) || /* round half to even */
        (tmp - mantissa > 0.5f))
    {
        mantissa++; /* round to nearest, away from zero */
    }
    if (mantissa == 2048)
    {
        mantissa = 1024;
        exp++;
    }

    if (exp > 31)
    {
        /* too big */
        ret = 0x7fff; /* INF */
    }
    else if (exp <= 0)
    {
        unsigned int rounding = 0;

        /* Denormalized half float */

        /* return 0x0000 (=0.0) for numbers too small to represent in half floats */
        if (exp < -11)
            return (sign ? 0x8000 : 0x0000);

        exp = origexp;

        /* the 13 extra bits from single precision are used for rounding */
        mantissa = (unsigned int)(tmp * powf(2, 13));
        mantissa >>= 1 - exp; /* denormalize */

        mantissa -= ~(mantissa >> 13) & 1; /* round half to even */
        /* remove 13 least significant bits to get half float precision */
        mantissa >>= 12;
        rounding = mantissa & 1;
        mantissa >>= 1;

        ret = mantissa + rounding;
    }
    else
    {
        ret = (exp << 10) | (mantissa & 0x3ff);
    }

    ret |= ((sign ? 1 : 0) << 15); /* Add the sign */
    return ret;
}

D3DXFLOAT16 *WINAPI D3DXFloat32To16Array(D3DXFLOAT16 *pout, CONST FLOAT *pin, UINT n)
{
    unsigned int i;

    TRACE("(%p, %p, %u)\n", pout, pin, n);

    for (i = 0; i < n; ++i)
    {
        pout[i].value = float_32_to_16(pin[i]);
    }

    return pout;
}

/* Native d3dx9's D3DXFloat16to32Array lacks support for NaN and Inf. Specifically, e = 16 is treated as a
 * regular number - e.g., 0x7fff is converted to 131008.0 and 0xffff to -131008.0. */
static inline float float_16_to_32(const unsigned short in)
{
    const unsigned short s = (in & 0x8000);
    const unsigned short e = (in & 0x7C00) >> 10;
    const unsigned short m = in & 0x3FF;
    const float sgn = (s ? -1.0f : 1.0f);

    if (e == 0)
    {
        if (m == 0) return sgn * 0.0f; /* +0.0 or -0.0 */
        else return sgn * powf(2, -14.0f) * (m / 1024.0f);
    }
    else
    {
        return sgn * powf(2, e - 15.0f) * (1.0f + (m / 1024.0f));
    }
}

FLOAT *WINAPI D3DXFloat16To32Array(FLOAT *pout, CONST D3DXFLOAT16 *pin, UINT n)
{
    unsigned int i;

    TRACE("(%p, %p, %u)\n", pout, pin, n);

    for (i = 0; i < n; ++i)
    {
        pout[i] = float_16_to_32(pin[i].value);
    }

    return pout;
}

/*_________________D3DXSH________________*/

FLOAT* WINAPI D3DXSHAdd(FLOAT *out, UINT order, const FLOAT *a, const FLOAT *b)
{
    UINT i;

    TRACE("out %p, order %u, a %p, b %p\n", out, order, a, b);

    for (i = 0; i < order * order; i++)
        out[i] = a[i] + b[i];

    return out;
}

FLOAT WINAPI D3DXSHDot(UINT order, CONST FLOAT *a, CONST FLOAT *b)
{
    FLOAT s;
    UINT i;

    TRACE("order %u, a %p, b %p\n", order, a, b);

    s = a[0] * b[0];
    for (i = 1; i < order * order; i++)
        s += a[i] * b[i];

    return s;
}

FLOAT* WINAPI D3DXSHEvalDirection(FLOAT *out, UINT order, CONST D3DXVECTOR3 *dir)
{

    TRACE("(%p, %u, %p)\n", out, order, dir);

    if ( (order < D3DXSH_MINORDER) || (order > D3DXSH_MAXORDER) )
        return out;

    out[0] = 0.5f / sqrt(D3DX_PI);
    out[1] = -0.5f / sqrt(D3DX_PI / 3.0f) * dir->y;
    out[2] = 0.5f / sqrt(D3DX_PI / 3.0f) * dir->z;
    out[3] = -0.5f / sqrt(D3DX_PI / 3.0f) * dir->x;
    if ( order == 2 )
        return out;

    out[4] = 0.5f / sqrt(D3DX_PI / 15.0f) * dir->x * dir->y;
    out[5] = -0.5f / sqrt(D3DX_PI / 15.0f) * dir->y * dir->z;
    out[6] = 0.25f / sqrt(D3DX_PI / 5.0f) * ( 3.0f * dir->z * dir->z - 1.0f );
    out[7] = -0.5f / sqrt(D3DX_PI / 15.0f) * dir->x * dir->z;
    out[8] = 0.25f / sqrt(D3DX_PI / 15.0f) * ( dir->x * dir->x - dir->y * dir->y );
    if ( order == 3 )
        return out;

    out[9] = -sqrt(70.0f / D3DX_PI) / 8.0f * dir->y * (3.0f * dir->x * dir->x - dir->y * dir->y );
    out[10] = sqrt(105.0f / D3DX_PI) / 2.0f * dir->x * dir->y * dir->z;
    out[11] = -sqrt(42.0 / D3DX_PI) / 8.0f * dir->y * ( -1.0f + 5.0f * dir->z * dir->z );
    out[12] = sqrt(7.0f / D3DX_PI) / 4.0f * dir->z * ( 5.0f * dir->z * dir->z - 3.0f );
    out[13] = sqrt(42.0 / D3DX_PI) / 8.0f * dir->x * ( 1.0f - 5.0f * dir->z * dir->z );
    out[14] = sqrt(105.0f / D3DX_PI) / 4.0f * dir->z * ( dir->x * dir->x - dir->y * dir->y );
    out[15] = -sqrt(70.0f / D3DX_PI) / 8.0f * dir->x * ( dir->x * dir->x - 3.0f * dir->y * dir->y );
    if ( order == 4 )
        return out;

    out[16] = 0.75f * sqrt(35.0f / D3DX_PI) * dir->x * dir->y * (dir->x * dir->x - dir->y * dir->y );
    out[17] = 3.0f * dir->z * out[9];
    out[18] = 0.75f * sqrt(5.0f / D3DX_PI) * dir->x * dir->y * ( 7.0f * dir->z * dir->z - 1.0f );
    out[19] = 0.375f * sqrt(10.0f / D3DX_PI) * dir->y * dir->z * ( 3.0f - 7.0f * dir->z * dir->z );
    out[20] = 3.0f / ( 16.0f * sqrt(D3DX_PI) ) * ( 35.0f * dir->z * dir->z * dir->z * dir->z - 30.f * dir->z * dir->z + 3.0f );
    out[21] = 0.375f * sqrt(10.0f / D3DX_PI) * dir->x * dir->z * ( 3.0f - 7.0f * dir->z * dir->z );
    out[22] = 0.375f * sqrt(5.0f / D3DX_PI) * ( dir->x * dir->x - dir->y * dir->y ) * ( 7.0f * dir->z * dir->z - 1.0f);
    out[23] = 3.0 * dir->z * out[15];
    out[24] = 3.0f / 16.0f * sqrt(35.0f / D3DX_PI) * ( dir->x * dir->x * dir->x * dir->x- 6.0f * dir->x * dir->x * dir->y * dir->y + dir->y * dir->y * dir->y * dir->y );
    if ( order == 5 )
        return out;

    out[25] = -3.0f/ 32.0f * sqrt(154.0f / D3DX_PI) * dir->y * ( 5.0f * dir->x * dir->x * dir->x * dir->x - 10.0f * dir->x * dir->x * dir->y * dir->y + dir->y * dir->y * dir->y * dir->y );
    out[26] = 0.75f * sqrt(385.0f / D3DX_PI) * dir->x * dir->y * dir->z * ( dir->x * dir->x - dir->y * dir->y );
    out[27] = sqrt(770.0f / D3DX_PI) / 32.0f * dir->y * ( 3.0f * dir->x * dir->x - dir->y * dir->y ) * ( 1.0f - 9.0f * dir->z * dir->z );
    out[28] = sqrt(1155.0f / D3DX_PI) / 4.0f * dir->x * dir->y * dir->z * ( 3.0f * dir->z * dir->z - 1.0f);
    out[29] = sqrt(165.0f / D3DX_PI) / 16.0f * dir->y * ( 14.0f * dir->z * dir->z - 21.0f * dir->z * dir->z * dir->z * dir->z - 1.0f );
    out[30] = sqrt(11.0f / D3DX_PI) / 16.0f * dir->z * ( 63.0f * dir->z * dir->z * dir->z * dir->z - 70.0f * dir->z * dir->z + 15.0f );
    out[31] = sqrt(165.0f / D3DX_PI) / 16.0f * dir->x * ( 14.0f * dir->z * dir->z - 21.0f * dir->z * dir->z * dir->z * dir->z - 1.0f );
    out[32] = sqrt(1155.0f / D3DX_PI) / 8.0f * dir->z * ( dir->x * dir->x - dir->y * dir->y ) * ( 3.0f * dir->z * dir->z - 1.0f );
    out[33] = sqrt(770.0f / D3DX_PI) / 32.0f * dir->x * ( dir->x * dir->x - 3.0f * dir->y * dir->y ) * ( 1.0f - 9.0f * dir->z * dir->z );
    out[34] = 3.0f / 16.0f * sqrt(385.0f / D3DX_PI) * dir->z * ( dir->x * dir->x * dir->x * dir->x - 6.0 * dir->x * dir->x * dir->y * dir->y + dir->y * dir->y * dir->y * dir->y );
    out[35] = -3.0f/ 32.0f * sqrt(154.0f / D3DX_PI) * dir->x * ( dir->x * dir->x * dir->x * dir->x - 10.0f * dir->x * dir->x * dir->y * dir->y + 5.0f * dir->y * dir->y * dir->y * dir->y );

    return out;
}

HRESULT WINAPI D3DXSHEvalDirectionalLight(UINT order, CONST D3DXVECTOR3 *dir, FLOAT Rintensity, FLOAT Gintensity, FLOAT Bintensity, FLOAT *Rout, FLOAT *Gout, FLOAT *Bout)
{
    FLOAT s, temp;
    UINT j;

    TRACE("Order %u, Vector %p, Red %f, Green %f, Blue %f, Rout %p, Gout %p, Bout %p\n", order, dir, Rintensity, Gintensity, Bintensity, Rout, Gout, Bout);

    s = 0.75f;
    if ( order > 2 )
        s += 5.0f / 16.0f;
    if ( order > 4 )
        s -= 3.0f / 32.0f;
    s /= D3DX_PI;

    D3DXSHEvalDirection(Rout, order, dir);
    for (j = 0; j < order * order; j++)
    {
        temp = Rout[j] / s;

        Rout[j] = Rintensity * temp;
        if ( Gout )
            Gout[j] = Gintensity * temp;
        if ( Bout )
            Bout[j] = Bintensity * temp;
    }

    return D3D_OK;
}

FLOAT * WINAPI D3DXSHMultiply2(FLOAT *out, const FLOAT *a, const FLOAT *b)
{
    FLOAT ta, tb;

    TRACE("out %p, a %p, b %p\n", out, a, b);

    ta = 0.28209479f * a[0];
    tb = 0.28209479f * b[0];

    out[0] = 0.28209479f * D3DXSHDot(2, a, b);
    out[1] = ta * b[1] + tb * a[1];
    out[2] = ta * b[2] + tb * a[2];
    out[3] = ta * b[3] + tb * a[3];

    return out;
}

FLOAT * WINAPI D3DXSHMultiply3(FLOAT *out, const FLOAT *a, const FLOAT *b)
{
    FLOAT t, ta, tb;

    TRACE("out %p, a %p, b %p\n", out, a, b);

    out[0] = 0.28209479f * a[0] * b[0];

    ta = 0.28209479f * a[0] - 0.12615662f * a[6] - 0.21850968f * a[8];
    tb = 0.28209479f * b[0] - 0.12615662f * b[6] - 0.21850968f * b[8];
    out[1] = ta * b[1] + tb * a[1];
    t = a[1] * b[1];
    out[0] += 0.28209479f * t;
    out[6] = -0.12615662f * t;
    out[8] = -0.21850968f * t;

    ta = 0.21850968f * a[5];
    tb = 0.21850968f * b[5];
    out[1] += ta * b[2] + tb * a[2];
    out[2] = ta * b[1] + tb * a[1];
    t = a[1] * b[2] +a[2] * b[1];
    out[5] = 0.21850968f * t;

    ta = 0.21850968f * a[4];
    tb = 0.21850968f * b[4];
    out[1] += ta * b[3] + tb * a[3];
    out[3]  = ta * b[1] + tb * a[1];
    t = a[1] * b[3] + a[3] * b[1];
    out[4] = 0.21850968f * t;

    ta = 0.28209480f * a[0] + 0.25231326f * a[6];
    tb = 0.28209480f * b[0] + 0.25231326f * b[6];
    out[2] += ta * b[2] + tb * a[2];
    t = a[2] * b[2];
    out[0] += 0.28209480f * t;
    out[6] += 0.25231326f * t;

    ta = 0.21850969f * a[7];
    tb = 0.21850969f * b[7];
    out[2] += ta * b[3] + tb * a[3];
    out[3] += ta * b[2] + tb * a[2];
    t = a[2] * b[3] + a[3] * b[2];
    out[7] = 0.21850969f * t;

    ta = 0.28209479f * a[0] - 0.12615663f * a[6] + 0.21850969f * a[8];
    tb = 0.28209479f * b[0] - 0.12615663f * b[6] + 0.21850969f * b[8];
    out[3] += ta * b[3] + tb * a[3];
    t = a[3] * b[3];
    out[0] += 0.28209479f * t;
    out[6] -= 0.12615663f * t;
    out[8] += 0.21850969f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[4] += ta * b[4] + tb * a[4];
    t = a[4] * b[4];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    ta = 0.15607835f * a[7];
    tb = 0.15607835f * b[7];
    out[4] += ta * b[5] + tb * a[5];
    out[5] += ta * b[4] + tb * a[4];
    t = a[4] * b[5] + a[5] * b[4];
    out[7] += 0.15607834f * t;

    ta = 0.28209479f * a[0] + 0.09011186 * a[6] - 0.15607835f * a[8];
    tb = 0.28209479f * b[0] + 0.09011186 * b[6] - 0.15607835f * b[8];
    out[5] += ta * b[5] + tb * a[5];
    t = a[5] * b[5];
    out[0] += 0.28209479f * t;
    out[6] += 0.09011186f * t;
    out[8] -= 0.15607835f * t;

    ta = 0.28209480f * a[0];
    tb = 0.28209480f * b[0];
    out[6] += ta * b[6] + tb * a[6];
    t = a[6] * b[6];
    out[0] += 0.28209480f * t;
    out[6] += 0.18022376f * t;

    ta = 0.28209479f * a[0] + 0.09011186 * a[6] + 0.15607835f * a[8];
    tb = 0.28209479f * b[0] + 0.09011186 * b[6] + 0.15607835f * b[8];
    out[7] += ta * b[7] + tb * a[7];
    t = a[7] * b[7];
    out[0] += 0.28209479f * t;
    out[6] += 0.09011186f * t;
    out[8] += 0.15607835f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[8] += ta * b[8] + tb * a[8];
    t = a[8] * b[8];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    return out;
}

FLOAT * WINAPI D3DXSHMultiply4(FLOAT *out, CONST FLOAT *a, CONST FLOAT *b)
{
    FLOAT ta, tb, t;

    TRACE("out %p, a %p, b %p\n", out, a, b);

    out[0] = 0.28209479f * a[0] * b[0];

    ta = 0.28209479f * a[0] - 0.12615663f * a[6] - 0.21850969f * a[8];
    tb = 0.28209479f * b[0] - 0.12615663f * b[6] - 0.21850969f * b[8];
    out[1] = ta * b[1] + tb * a[1];
    t = a[1] * b[1];
    out[0] += 0.28209479f * t;
    out[6] = -0.12615663f * t;
    out[8] = -0.21850969f * t;

    ta = 0.21850969f * a[3] - 0.05839917f * a[13] - 0.22617901f * a[15];
    tb = 0.21850969f * b[3] - 0.05839917f * b[13] - 0.22617901f * b[15];
    out[1] += ta * b[4] + tb * a[4];
    out[4] = ta * b[1] + tb * a[1];
    t = a[1] * b[4] + a[4] * b[1];
    out[3] = 0.21850969f * t;
    out[13] = -0.05839917f * t;
    out[15] = -0.22617901f * t;

    ta = 0.21850969f * a[2] - 0.14304817f * a[12] - 0.18467439f * a[14];
    tb = 0.21850969f * b[2] - 0.14304817f * b[12] - 0.18467439f * b[14];
    out[1] += ta * b[5] + tb * a[5];
    out[5] = ta * b[1] + tb * a[1];
    t = a[1] * b[5] + a[5] * b[1];
    out[2] = 0.21850969f * t;
    out[12] = -0.14304817f * t;
    out[14] = -0.18467439f * t;

    ta = 0.20230066f * a[11];
    tb = 0.20230066f * b[11];
    out[1] += ta * b[6] + tb * a[6];
    out[6] += ta * b[1] + tb * a[1];
    t = a[1] * b[6] + a[6] * b[1];
    out[11] = 0.20230066f * t;

    ta = 0.22617901f * a[9] + 0.05839917f * a[11];
    tb = 0.22617901f * b[9] + 0.05839917f * b[11];
    out[1] += ta * b[8] + tb * a[8];
    out[8] += ta * b[1] + tb * a[1];
    t = a[1] * b[8] + a[8] * b[1];
    out[9] = 0.22617901f * t;
    out[11] += 0.05839917f * t;

    ta = 0.28209480f * a[0] + 0.25231326f * a[6];
    tb = 0.28209480f * b[0] + 0.25231326f * b[6];
    out[2] += ta * b[2] + tb * a[2];
    t = a[2] * b[2];
    out[0] += 0.28209480f * t;
    out[6] += 0.25231326f * t;

    ta = 0.24776671f * a[12];
    tb = 0.24776671f * b[12];
    out[2] += ta * b[6] + tb * a[6];
    out[6] += ta * b[2] + tb * a[2];
    t = a[2] * b[6] + a[6] * b[2];
    out[12] += 0.24776671f * t;

    ta = 0.28209480f * a[0] - 0.12615663f * a[6] + 0.21850969f * a[8];
    tb = 0.28209480f * b[0] - 0.12615663f * b[6] + 0.21850969f * b[8];
    out[3] += ta * b[3] + tb * a[3];
    t = a[3] * b[3];
    out[0] += 0.28209480f * t;
    out[6] -= 0.12615663f * t;
    out[8] += 0.21850969f * t;

    ta = 0.20230066f * a[13];
    tb = 0.20230066f * b[13];
    out[3] += ta * b[6] + tb * a[6];
    out[6] += ta * b[3] + tb * a[3];
    t = a[3] * b[6] + a[6] * b[3];
    out[13] += 0.20230066f * t;

    ta = 0.21850969f * a[2] - 0.14304817f * a[12] + 0.18467439f * a[14];
    tb = 0.21850969f * b[2] - 0.14304817f * b[12] + 0.18467439f * b[14];
    out[3] += ta * b[7] + tb * a[7];
    out[7] = ta * b[3] + tb * a[3];
    t = a[3] * b[7] + a[7] * b[3];
    out[2] += 0.21850969f * t;
    out[12] -= 0.14304817f * t;
    out[14] += 0.18467439f * t;

    ta = -0.05839917f * a[13] + 0.22617901f * a[15];
    tb = -0.05839917f * b[13] + 0.22617901f * b[15];
    out[3] += ta * b[8] + tb * a[8];
    out[8] += ta * b[3] + tb * a[3];
    t = a[3] * b[8] + a[8] * b[3];
    out[13] -= 0.05839917f * t;
    out[15] += 0.22617901f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[4] += ta * b[4] + tb * a[4];
    t = a[4] * b[4];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    ta = 0.15607835f * a[7];
    tb = 0.15607835f * b[7];
    out[4] += ta * b[5] + tb * a[5];
    out[5] += ta * b[4] + tb * a[4];
    t = a[4] * b[5] + a[5] * b[4];
    out[7] += 0.15607835f * t;

    ta = 0.22617901f * a[3] - 0.09403160f * a[13];
    tb = 0.22617901f * b[3] - 0.09403160f * b[13];
    out[4] += ta * b[9] + tb * a[9];
    out[9] += ta * b[4] + tb * a[4];
    t = a[4] * b[9] + a[9] * b[4];
    out[3] += 0.22617901f * t;
    out[13] -= 0.09403160f * t;

    ta = 0.18467439f * a[2] - 0.18806319f * a[12];
    tb = 0.18467439f * b[2] - 0.18806319f * b[12];
    out[4] += ta * b[10] + tb * a [10];
    out[10] = ta * b[4] + tb * a[4];
    t = a[4] * b[10] + a[10] * b[4];
    out[2] += 0.18467439f * t;
    out[12] -= 0.18806319f * t;

    ta = -0.05839917f * a[3] + 0.14567312f * a[13] + 0.09403160f * a[15];
    tb = -0.05839917f * b[3] + 0.14567312f * b[13] + 0.09403160f * b[15];
    out[4] += ta * b[11] + tb * a[11];
    out[11] += ta * b[4] + tb * a[4];
    t = a[4] * b[11] + a[11] * b[4];
    out[3] -= 0.05839917f * t;
    out[13] += 0.14567312f * t;
    out[15] += 0.09403160f * t;

    ta = 0.28209479f * a[0] + 0.09011186f * a[6] - 0.15607835f * a[8];
    tb = 0.28209479f * b[0] + 0.09011186f * b[6] - 0.15607835f * b[8];
    out[5] += ta * b[5] + tb * a[5];
    t = a[5] * b[5];
    out[0] += 0.28209479f * t;
    out[6] += 0.09011186f * t;
    out[8] -= 0.15607835f * t;

    ta = 0.14867701f * a[14];
    tb = 0.14867701f * b[14];
    out[5] += ta * b[9] + tb * a[9];
    out[9] += ta * b[5] + tb * a[5];
    t = a[5] * b[9] + a[9] * b[5];
    out[14] += 0.14867701f * t;

    ta = 0.18467439f * a[3] + 0.11516472f * a[13] - 0.14867701f * a[15];
    tb = 0.18467439f * b[3] + 0.11516472f * b[13] - 0.14867701f * b[15];
    out[5] += ta * b[10] + tb * a[10];
    out[10] += ta * b[5] + tb * a[5];
    t = a[5] * b[10] + a[10] * b[5];
    out[3] += 0.18467439f * t;
    out[13] += 0.11516472f * t;
    out[15] -= 0.14867701f * t;

    ta = 0.23359668f * a[2] + 0.05947080f * a[12] - 0.11516472f * a[14];
    tb = 0.23359668f * b[2] + 0.05947080f * b[12] - 0.11516472f * b[14];
    out[5] += ta * b[11] + tb * a[11];
    out[11] += ta * b[5] + tb * a[5];
    t = a[5] * b[11] + a[11] * b[5];
    out[2] += 0.23359668f * t;
    out[12] += 0.05947080f * t;
    out[14] -= 0.11516472f * t;

    ta = 0.28209479f * a[0];
    tb = 0.28209479f * b[0];
    out[6] += ta * b[6] + tb * a[6];
    t = a[6] * b[6];
    out[0] += 0.28209479f * t;
    out[6] += 0.18022376f * t;

    ta = 0.09011186f * a[6] + 0.28209479f * a[0] + 0.15607835f * a[8];
    tb = 0.09011186f * b[6] + 0.28209479f * b[0] + 0.15607835f * b[8];
    out[7] += ta * b[7] + tb * a[7];
    t = a[7] * b[7];
    out[6] += 0.09011186f * t;
    out[0] += 0.28209479f * t;
    out[8] += 0.15607835f * t;

    ta = 0.14867701f * a[9] + 0.18467439f * a[1] + 0.11516472f * a[11];
    tb = 0.14867701f * b[9] + 0.18467439f * b[1] + 0.11516472f * b[11];
    out[7] += ta * b[10] + tb * a[10];
    out[10] += ta * b[7] + tb * a[7];
    t = a[7] * b[10] + a[10] * b[7];
    out[9] += 0.14867701f * t;
    out[1] += 0.18467439f * t;
    out[11] += 0.11516472f * t;

    ta = 0.05947080f * a[12] + 0.23359668f * a[2] + 0.11516472f * a[14];
    tb = 0.05947080f * b[12] + 0.23359668f * b[2] + 0.11516472f * b[14];
    out[7] += ta * b[13] + tb * a[13];
    out[13] += ta * b[7]+ tb * a[7];
    t = a[7] * b[13] + a[13] * b[7];
    out[12] += 0.05947080f * t;
    out[2] += 0.23359668f * t;
    out[14] += 0.11516472f * t;

    ta = 0.14867701f * a[15];
    tb = 0.14867701f * b[15];
    out[7] += ta * b[14] + tb * a[14];
    out[14] += ta * b[7] + tb * a[7];
    t = a[7] * b[14] + a[14] * b[7];
    out[15] += 0.14867701f * t;

    ta = 0.28209479f * a[0] - 0.18022375f * a[6];
    tb = 0.28209479f * b[0] - 0.18022375f * b[6];
    out[8] += ta * b[8] + tb * a[8];
    t = a[8] * b[8];
    out[0] += 0.28209479f * t;
    out[6] -= 0.18022375f * t;

    ta = -0.09403160f * a[11];
    tb = -0.09403160f * b[11];
    out[8] += ta * b[9] + tb * a[9];
    out[9] += ta * b[8] + tb * a[8];
    t = a[8] * b[9] + a[9] * b[8];
    out[11] -= 0.09403160f * t;

    ta = -0.09403160f * a[15];
    tb = -0.09403160f * b[15];
    out[8] += ta * b[13] + tb * a[13];
    out[13] += ta * b[8] + tb * a[8];
    t = a[8] * b[13] + a[13] * b[8];
    out[15] -= 0.09403160f * t;

    ta = 0.18467439f * a[2] - 0.18806319f * a[12];
    tb = 0.18467439f * b[2] - 0.18806319f * b[12];
    out[8] += ta * b[14] + tb * a[14];
    out[14] += ta * b[8] + tb * a[8];
    t = a[8] * b[14] + a[14] * b[8];
    out[2] += 0.18467439f * t;
    out[12] -= 0.18806319f * t;

    ta = -0.21026104f * a[6] + 0.28209479f * a[0];
    tb = -0.21026104f * b[6] + 0.28209479f * b[0];
    out[9] += ta * b[9] + tb * a[9];
    t = a[9] * b[9];
    out[6] -= 0.21026104f * t;
    out[0] += 0.28209479f * t;

    ta = 0.28209479f * a[0];
    tb = 0.28209479f * b[0];
    out[10] += ta * b[10] + tb * a[10];
    t = a[10] * b[10];
    out[0] += 0.28209479f * t;

    ta = 0.28209479f * a[0] + 0.12615663f * a[6] - 0.14567312f * a[8];
    tb = 0.28209479f * b[0] + 0.12615663f * b[6] - 0.14567312f * b[8];
    out[11] += ta * b[11] + tb * a[11];
    t = a[11] * b[11];
    out[0] += 0.28209479f * t;
    out[6] += 0.12615663f * t;
    out[8] -= 0.14567312f * t;

    ta = 0.28209479f * a[0] + 0.16820885f * a[6];
    tb = 0.28209479f * b[0] + 0.16820885f * b[6];
    out[12] += ta * b[12] + tb * a[12];
    t = a[12] * b[12];
    out[0] += 0.28209479f * t;
    out[6] += 0.16820885f * t;

    ta =0.28209479f * a[0] + 0.14567312f * a[8] + 0.12615663f * a[6];
    tb =0.28209479f * b[0] + 0.14567312f * b[8] + 0.12615663f * b[6];
    out[13] += ta * b[13] + tb * a[13];
    t = a[13] * b[13];
    out[0] += 0.28209479f * t;
    out[8] += 0.14567312f * t;
    out[6] += 0.12615663f * t;

    ta = 0.28209479f * a[0];
    tb = 0.28209479f * b[0];
    out[14] += ta * b[14] + tb * a[14];
    t = a[14] * b[14];
    out[0] += 0.28209479f * t;

    ta = 0.28209479f * a[0] - 0.21026104f * a[6];
    tb = 0.28209479f * b[0] - 0.21026104f * b[6];
    out[15] += ta * b[15] + tb * a[15];
    t = a[15] * b[15];
    out[0] += 0.28209479f * t;
    out[6] -= 0.21026104f * t;

    return out;
}

static void rotate_X(FLOAT *out, UINT order, FLOAT a, FLOAT *in)
{
    out[0] = in[0];

    out[1] = a * in[2];
    out[2] = -a * in[1];
    out[3] = in[3];

    out[4] = a * in[7];
    out[5] = -in[5];
    out[6] = -0.5f * in[6] - 0.8660253882f * in[8];
    out[7] = -a * in[4];
    out[8] = -0.8660253882f * in[6] + 0.5f * in[8];
    out[9] = -a * 0.7905694842f * in[12] + a * 0.6123724580f * in[14];

    out[10] = -in[10];
    out[11] = -a * 0.6123724580f * in[12] - a * 0.7905694842f * in[14];
    out[12] = a * 0.7905694842f * in[9] + a * 0.6123724580f * in[11];
    out[13] = -0.25f * in[13] - 0.9682458639f * in[15];
    out[14] = -a * 0.6123724580f * in[9] + a * 0.7905694842f * in[11];
    out[15] = -0.9682458639f * in[13] + 0.25f * in[15];
    if (order == 4)
        return;

    out[16] = -a * 0.9354143739f * in[21] + a * 0.3535533845f * in[23];
    out[17] = -0.75f * in[17] + 0.6614378095f * in[19];
    out[18] = -a * 0.3535533845f * in[21] - a * 0.9354143739f * in[23];
    out[19] = 0.6614378095f * in[17] + 0.75f * in[19];
    out[20] = 0.375f * in[20] + 0.5590170026f * in[22] + 0.7395099998f * in[24];
    out[21] = a * 0.9354143739f * in[16] + a * 0.3535533845f * in[18];
    out[22] = 0.5590170026f * in[20] + 0.5f * in[22] - 0.6614378691f * in[24];
    out[23] = -a * 0.3535533845f * in[16] + a * 0.9354143739f * in[18];
    out[24] = 0.7395099998f * in[20] - 0.6614378691f * in[22] + 0.125f * in[24];
    if (order == 5)
        return;

    out[25] = a * 0.7015607357f * in[30] - a * 0.6846531630f * in[32] + a * 0.1976423711f * in[34];
    out[26] = -0.5f * in[26] + 0.8660253882f * in[28];
    out[27] = a * 0.5229125023f * in[30] + a * 0.3061861992f * in[32] - a * 0.7954951525 * in[34];
    out[28] = 0.8660253882f * in[26] + 0.5f * in[28];
    out[29] = a * 0.4841229022f * in[30] + a * 0.6614378691f * in[32] + a * 0.5728219748f * in[34];
    out[30] = -a * 0.7015607357f * in[25] - a * 0.5229125023f * in[27] - a * 0.4841229022f * in[29];
    out[31] = 0.125f * in[31] + 0.4050463140f * in[33] + 0.9057110548f * in[35];
    out[32] = a * 0.6846531630f * in[25] - a * 0.3061861992f * in[27] - a * 0.6614378691f * in[29];
    out[33] = 0.4050463140f * in[31] + 0.8125f * in[33] - 0.4192627370f * in[35];
    out[34] = -a * 0.1976423711f * in[25] + a * 0.7954951525f * in[27] - a * 0.5728219748f * in[29];
    out[35] = 0.9057110548f * in[31] - 0.4192627370f * in[33] + 0.0624999329f * in[35];
}

FLOAT* WINAPI D3DXSHRotate(FLOAT *out, UINT order, CONST D3DXMATRIX *matrix, CONST FLOAT *in)
{
    FLOAT alpha, beta, gamma, sinb, temp[36], temp1[36];

    TRACE("out %p, order %u, matrix %p, in %p\n", out, order, matrix, in);

    out[0] = in[0];

    if ((order > D3DXSH_MAXORDER) || (order < D3DXSH_MINORDER))
        return out;

    if (order <= 3)
    {
        out[1] = matrix->u.m[1][1] * in[1] - matrix->u.m[2][1] * in[2] + matrix->u.m[0][1] * in[3];
        out[2] = -matrix->u.m[1][2] * in[1] + matrix->u.m[2][2] * in[2] - matrix->u.m[0][2] * in[3];
        out[3] = matrix->u.m[1][0] * in[1] - matrix->u.m[2][0] * in[2] + matrix->u.m[0][0] * in[3];

        if (order == 3)
        {
            FLOAT coeff[]={
                matrix->u.m[1][0] * matrix->u.m[0][0], matrix->u.m[1][1] * matrix->u.m[0][1],
                matrix->u.m[1][1] * matrix->u.m[2][1], matrix->u.m[1][0] * matrix->u.m[2][0],
                matrix->u.m[2][0] * matrix->u.m[2][0], matrix->u.m[2][1] * matrix->u.m[2][1],
                matrix->u.m[0][0] * matrix->u.m[2][0], matrix->u.m[0][1] * matrix->u.m[2][1],
                matrix->u.m[0][1] * matrix->u.m[0][1], matrix->u.m[1][0] * matrix->u.m[1][0],
                matrix->u.m[1][1] * matrix->u.m[1][1], matrix->u.m[0][0] * matrix->u.m[0][0], };

            out[4] = (matrix->u.m[1][1] * matrix->u.m[0][0] + matrix->u.m[0][1] * matrix->u.m[1][0]) * in[4];
            out[4] -= (matrix->u.m[1][0] * matrix->u.m[2][1] + matrix->u.m[1][1] * matrix->u.m[2][0]) * in[5];
            out[4] += 1.7320508076f * matrix->u.m[2][0] * matrix->u.m[2][1] * in[6];
            out[4] -= (matrix->u.m[0][1] * matrix->u.m[2][0] + matrix->u.m[0][0] * matrix->u.m[2][1]) * in[7];
            out[4] += (matrix->u.m[0][0] * matrix->u.m[0][1] - matrix->u.m[1][0] * matrix->u.m[1][1]) * in[8];

            out[5] = (matrix->u.m[1][1] * matrix->u.m[2][2] + matrix->u.m[1][2] * matrix->u.m[2][1]) * in[5];
            out[5] -= (matrix->u.m[1][1] * matrix->u.m[0][2] + matrix->u.m[1][2] * matrix->u.m[0][1]) * in[4];
            out[5] -= 1.7320508076f * matrix->u.m[2][2] * matrix->u.m[2][1] * in[6];
            out[5] += (matrix->u.m[0][2] * matrix->u.m[2][1] + matrix->u.m[0][1] * matrix->u.m[2][2]) * in[7];
            out[5] -= (matrix->u.m[0][1] * matrix->u.m[0][2] - matrix->u.m[1][1] * matrix->u.m[1][2]) * in[8];

            out[6] = (matrix->u.m[2][2] * matrix->u.m[2][2] - 0.5f * (coeff[4] + coeff[5])) * in[6];
            out[6] -= (0.5773502692f * (coeff[0] + coeff[1]) - 1.1547005384f * matrix->u.m[1][2] * matrix->u.m[0][2]) * in[4];
            out[6] += (0.5773502692f * (coeff[2] + coeff[3]) - 1.1547005384f * matrix->u.m[1][2] * matrix->u.m[2][2]) * in[5];
            out[6] += (0.5773502692f * (coeff[6] + coeff[7]) - 1.1547005384f * matrix->u.m[0][2] * matrix->u.m[2][2]) * in[7];
            out[6] += (0.2886751347f * (coeff[9] - coeff[8] + coeff[10] - coeff[11]) - 0.5773502692f *
                  (matrix->u.m[1][2] * matrix->u.m[1][2] - matrix->u.m[0][2] * matrix->u.m[0][2])) * in[8];

            out[7] = (matrix->u.m[0][0] * matrix->u.m[2][2] + matrix->u.m[0][2] * matrix->u.m[2][0]) * in[7];
            out[7] -= (matrix->u.m[1][0] * matrix->u.m[0][2] + matrix->u.m[1][2] * matrix->u.m[0][0]) * in[4];
            out[7] += (matrix->u.m[1][0] * matrix->u.m[2][2] + matrix->u.m[1][2] * matrix->u.m[2][0]) * in[5];
            out[7] -= 1.7320508076f * matrix->u.m[2][2] * matrix->u.m[2][0] * in[6];
            out[7] -= (matrix->u.m[0][0] * matrix->u.m[0][2] - matrix->u.m[1][0] * matrix->u.m[1][2]) * in[8];

            out[8] = 0.5f * (coeff[11] - coeff[8] - coeff[9] + coeff[10]) * in[8];
            out[8] += (coeff[0] - coeff[1]) * in[4];
            out[8] += (coeff[2] - coeff[3]) * in[5];
            out[8] += 0.86602540f * (coeff[4] - coeff[5]) * in[6];
            out[8] += (coeff[7] - coeff[6]) * in[7];
        }

        return out;
    }

    if (fabsf(matrix->u.m[2][2]) != 1.0f)
    {
        sinb = sqrtf(1.0f - matrix->u.m[2][2] * matrix->u.m[2][2]);
        alpha = atan2f(matrix->u.m[2][1] / sinb, matrix->u.m[2][0] / sinb);
        beta = atan2f(sinb, matrix->u.m[2][2]);
        gamma = atan2f(matrix->u.m[1][2] / sinb, -matrix->u.m[0][2] / sinb);
    }
    else
    {
        alpha = atan2f(matrix->u.m[0][1], matrix->u.m[0][0]);
        beta = 0.0f;
        gamma = 0.0f;
    }

    D3DXSHRotateZ(temp, order, gamma, in);
    rotate_X(temp1, order, 1.0f, temp);
    D3DXSHRotateZ(temp, order, beta, temp1);
    rotate_X(temp1, order, -1.0f, temp);
    D3DXSHRotateZ(out, order, alpha, temp1);

    return out;
}

FLOAT * WINAPI D3DXSHRotateZ(FLOAT *out, UINT order, FLOAT angle, const FLOAT *in)
{
    UINT i, sum = 0;
    FLOAT c[5], s[5];

    TRACE("out %p, order %u, angle %f, in %p\n", out, order, angle, in);

    order = min(max(order, D3DXSH_MINORDER), D3DXSH_MAXORDER);

    out[0] = in[0];

    for (i = 1; i < order; i++)
    {
        UINT j;

        c[i - 1] = cosf(i * angle);
        s[i - 1] = sinf(i * angle);
        sum += i * 2;

        out[sum - i] = c[i - 1] * in[sum - i];
        out[sum - i] += s[i - 1] * in[sum + i];
        for (j = i - 1; j > 0; j--)
        {
            out[sum - j] = 0.0f;
            out[sum - j] = c[j - 1] * in[sum - j];
            out[sum - j] += s[j - 1] * in[sum + j];
        }

        if (in == out)
            out[sum] = 0.0f;
        else
            out[sum] = in[sum];

        for (j = 1; j < i; j++)
        {
            out[sum + j] = 0.0f;
            out[sum + j] = -s[j - 1] * in[sum - j];
            out[sum + j] += c[j - 1] * in[sum + j];
        }
        out[sum + i] = -s[i - 1] * in[sum - i];
        out[sum + i] += c[i - 1] * in[sum + i];
    }

    return out;
}

FLOAT* WINAPI D3DXSHScale(FLOAT *out, UINT order, CONST FLOAT *a, CONST FLOAT scale)
{
    UINT i;

    TRACE("out %p, order %u, a %p, scale %f\n", out, order, a, scale);

    for (i = 0; i < order * order; i++)
        out[i] = a[i] * scale;

    return out;
}
