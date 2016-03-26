//******************************************************************************
//  Project:		Weather-based simulation framework (WBSF)
//	Programmer:     Rémi Saint-Amant
// 
//  It under the terms of the GNU General Public License as published by
//     the Free Software Foundation
//  It is provided "as is" without express or implied warranty.
//******************************************************************************
#pragma once

#include <memory>
#include <boost\dynamic_bitset.hpp>

#include "basic/ERMsg.h"
#include "Basic/DBF3.h"
#include "Basic/GeoBasic.h"
#include "Basic/UtilStd.h"
#include "Geomatic/Projection.h"
#include "Geomatic/ShapeFileHeader.h"
#include "Geomatic/SFBoudingBox.h"


namespace WBSF
{

	class CProjection;

	class CProjectionTransformation;
	class CShapeFileIndex;
	class CShapeFileBase;
	class CSFPoint;


	typedef std::shared_ptr<CShapeFileBase> CShapeFileBasePtr;



	//**************************************************************
	class CSFShape
	{
	public:
		CSFShape();
		virtual ~CSFShape();

		__int32 GetType()const{ return m_shapeType; }
		virtual __int32 GetTypeNumber()const;
		virtual __int32 GetLength()const;
		virtual CSFShape* GetCopy()const;

		template<class Archive>
		Archive& Read(Archive& io)
		{
			return io;
		}

		template<class Archive>
		Archive& Write(Archive& io)const
		{
			return io;
		}

		virtual void clear();

		virtual void GetBoundingBox(CGeoRect& box)const;
		virtual bool AjoutePoint(const CGeoPoint& point);
		virtual void InsertPoint(int segmentNo, const CGeoPoint& point);
		virtual void RemovePoint(int index);
		virtual double GetMinimumDistance(const CGeoPoint& pt, int* pNearestSegmentNo = NULL)const;
		virtual bool SometingInRect(const CGeoRect& rect)const;
		virtual bool IsInside(const CGeoPoint& P)const;

		int GetNearestPointNo(const CGeoPoint& pt)const;
		int GetNearestSegmentNo(const CGeoPoint& pt)const;


		virtual void SetPrjID(size_t prjID);
		virtual bool TransformeProjection(CProjectionTransformation const& PT);

		virtual void GetCentroid(CGeoPoint& pt)const;
		virtual double GetArea(int ringNo = -1)const;


		virtual void RemoveRing(int ringNo);
		virtual int GetRingNo(int nodeNo)const;
		virtual void InversePointOrder(int ringNo);

		virtual int NewRing(){ return 0; }
		virtual int GetNbRing()const{ return 0; }
		virtual void GetPointIndex(int ringNo, int& first, int& last)const{ ASSERT(false); }
		virtual int GetNbPoints()const{ return 0; }
		virtual const CSFPoint& GetPoint(int index)const;
		virtual void SetPoint(int index, const CSFPoint& point, bool bAjustRing = false){}



		void GetAdjustedBoundingBox(CSFBoundingBox& boudingBox)const;
	protected:

		static double SQUARE(const double& x){ return x*x; }
		static double SIGNE(const double& x){ return (x >= 0) ? 1 : -1; }

		__int32 m_shapeType;
	};

	typedef std::vector<CSFShape&> CSFShapeArray;




	//**************************************************************
	class CSFPoint : public CGeoPoint, public CSFShape
	{
	public:

		CSFPoint(size_t prjID = PRJ_NOT_INIT);
		CSFPoint(const CGeoPoint& pt);
		CSFPoint(const double& x, const double& y, size_t prjID);
		virtual ~CSFPoint();

		CSFPoint& operator=(const CGeoPoint& point);
		static const CSFPoint POINT_NULL;

		virtual CSFShape* GetCopy()const;
		virtual __int32 GetLength()const;
		template<class Archive>
		Archive& Read(Archive& io)
		{
			io >> m_x;
			io >> m_y;

			return io;
		}

		template<class Archive>
		Archive& Write(Archive& io)const
		{
			io << m_x;
			io << m_y;

			return io;
		}
		bool operator == (const CGeoPoint& point)const;
		bool operator != (const CGeoPoint& point)const;
		virtual void clear();
		virtual void GetBoundingBox(CGeoRect& box)const;
		virtual bool AjoutePoint(const CGeoPoint& point);
		virtual void InsertPoint(int segmentNo, const CGeoPoint& point);
		virtual void RemovePoint(int index);
		virtual double GetMinimumDistance(const CGeoPoint& pt, int* pNearestSegmentNo = NULL)const;

		virtual void SetPrjID(size_t prjID);
		virtual bool TransformeProjection(CProjectionTransformation const& PT);
		virtual void GetCentroid(CGeoPoint& pt)const;

		virtual bool SometingInRect(const CGeoRect& rect)const;
		double Pente(const CGeoPoint& p)const{ return (p.m_y - m_y) / (p.m_x - m_x); }

		virtual void GetPointIndex(int ringNo, int& first, int& last)const{ first = last = 0; }
		virtual int GetNbRing()const{ return 1; }
		virtual int GetNbPoints()const{ return 1; }
		virtual const CSFPoint& GetPoint(int index)const{ return *this; }
		virtual void SetPoint(int index, const CSFPoint& point, bool bAjustRing = false){ *this = point; }


	};

	typedef std::vector<CSFPoint> CSFPointArray;



	//**************************************************************
	class CSFMultiPoint : public CSFShape
	{
	public:

		CSFMultiPoint();
		~CSFMultiPoint();

		virtual CSFShape* GetCopy()const;
		virtual __int32 GetLength()const;

		template<class Archive>
		Archive& Read(Archive& io)
		{
			m_boundingBox.Read(io);
			__int32 numPoints = 0;
			io >> numPoints;

			m_points.resize(numPoints);
			for (int i = 0; i < numPoints; i++)
				m_points[i].Read(io);

			return io;
		}

		template<class Archive>
		Archive& Write(Archive& io)const
		{
			m_boundingBox.Write(io);
			__int32 numPoints = (__int32)m_points.size();

			io << numPoints;

			for (int i = 0; i < numPoints; i++)
				m_points[i].Write(io);


			return io;
		}

		virtual void clear();
		virtual void GetBoundingBox(CGeoRect& box)const;
		virtual double GetMinimumDistance(const CGeoPoint& pt, int* pNearestSegmentNo = NULL)const;

		//virtual bool Draw(CDC* pDC, const CRect& rcBounds, const CShowViewport& viewPort, const CShapeFileBase& shapeFile);
		virtual void SetPrjID(size_t prjID);
		virtual bool TransformeProjection(CProjectionTransformation const& PT);
		virtual void GetCentroid(CGeoPoint& pt)const;
		virtual double GetArea(int ringNo = -1)const;

		virtual bool AjoutePoint(const CGeoPoint& point);
		virtual void InsertPoint(int segmentNo, const CGeoPoint& point);
		virtual void RemovePoint(int index);
		inline const CSFPointArray& GetPointsArray()const;

		virtual void GetRingPoints(int ring, CSFPointArray& point)const;
		virtual void SetRingPoints(int ring, const CSFPointArray& point);
		virtual void AddRing(const CSFPointArray& point);
		virtual inline void GetPointIndex(int ringNo, int& first, int& last)const;
		virtual inline int GetNbPoints()const;
		virtual inline const CSFPoint& GetPoint(int index)const;
		virtual inline void SetPoint(int index, const CSFPoint& point, bool bAjustRing = false);
		virtual bool SometingInRect(const CGeoRect& rect)const;

		virtual void InversePointOrder(int ringNo);

	protected:

		CSFBoundingBox m_boundingBox;
		CSFPointArray m_points;

	};

	inline const CSFPointArray& CSFMultiPoint::GetPointsArray()const
	{
		return m_points;
	}


	inline void CSFMultiPoint::GetPointIndex(int, int& first, int& last)const
	{
		first = 0;
		last = (int)m_points.size() - 1;
	}

	inline int CSFMultiPoint::GetNbPoints()const
	{
		return (int)m_points.size();
	}

	inline const CSFPoint& CSFMultiPoint::GetPoint(int index)const
	{
		return m_points[index];
	}

	inline void CSFMultiPoint::SetPoint(int index, const CSFPoint& point, bool)
	{
		m_points[index] = point;
		GetAdjustedBoundingBox(m_boundingBox);
	}

	//**************************************************************
	class CSFPolyLine : public CSFShape
	{
	public:

		CSFPolyLine();
		~CSFPolyLine();

		virtual CSFShape* GetCopy()const;
		virtual __int32 GetLength()const;

		template<class Archive>
		Archive& Read(Archive& io)
		{
			m_boundingBox.Read(io);
			__int32 numParts = 0;
			io >> numParts;
			__int32 numPoints = 0;
			io >> numPoints;

			m_beginParts.resize(numParts);
			for (int i = 0; i < numParts; i++)
				io >> m_beginParts[i];

			m_points.resize(numPoints);
			for (int i = 0; i < numPoints; i++)
				m_points[i].Read(io);

			return io;
		}

		template<class Archive>
		Archive& Write(Archive& io)const
		{
			m_boundingBox.Write(io);
			__int32 numParts = (__int32)m_beginParts.size();
			__int32 numPoints = (__int32)m_points.size();

			io << numParts;
			io << numPoints;

			for (int i = 0; i < numParts; i++)
				io << m_beginParts[i];

			for (int i = 0; i < numPoints; i++)
				m_points[i].Write(io);

			return io;
		}

		virtual void clear();
		virtual void GetBoundingBox(CGeoRect& box)const;
		virtual double GetMinimumDistance(const CGeoPoint& pt, int* pNearestSegmentNo = NULL)const;

		virtual void SetPrjID(size_t prjID);
		virtual bool TransformeProjection(CProjectionTransformation const& PT);
		virtual void GetCentroid(CGeoPoint& pt)const;
		virtual double GetArea(int ringNo = -1)const;

		virtual bool AjoutePoint(const CGeoPoint& point);
		virtual void InsertPoint(int segmentNo, const CGeoPoint& point);
		virtual void RemovePoint(int index);

		virtual void GetRingPoints(int ring, CSFPointArray& point)const;
		virtual void SetRingPoints(int ring, const CSFPointArray& point);
		virtual void AddRing(const CSFPointArray& point);

		virtual inline int NewRing();
		virtual inline int GetNbRing()const;
		virtual void RemoveRing(int ringNo);
		virtual int GetRingNo(int nodeNo)const;

		virtual inline void GetPointIndex(int ringNo, int& first, int& last)const;
		virtual inline int GetNbPoints()const;
		inline int GetNbPoints(int ringNo)const;
		virtual inline const CSFPoint& GetPoint(int index)const;
		inline const CSFPoint& GetPoint(int ringNo, int index)const;
		virtual inline void SetPoint(int index, const CSFPoint& point, bool bAjustRing = false);
		virtual bool SometingInRect(const CGeoRect& rect)const;
		virtual bool SometingInRect(int ringIndex, const CGeoRect& rect)const;


		virtual void InversePointOrder(int ringNo);

		int GetNbIntersection(const CGeoSegment& segment)const;
		void GetItersectionPoints(const CGeoSegment& segment, CSFPointArray& point)const;

	protected:


		void AdjusteBeginParts(int ringNo, int shift);


		CSFBoundingBox m_boundingBox;
		std::vector<int> m_beginParts;
		CSFPointArray m_points;

	};

	inline const CSFPoint& CSFPolyLine::GetPoint(int index)const
	{
		ASSERT(index >= 0 && index < m_points.size());

		return m_points[index];
	}
	inline const CSFPoint& CSFPolyLine::GetPoint(int ringNo, int index)const
	{
		int first = 0;
		int last = 0;
		GetPointIndex(ringNo, first, last);
		return GetPoint(first + index);
	}


	inline void CSFPolyLine::SetPoint(int index, const CSFPoint& point, bool)
	{
		m_points[index] = point;
		GetAdjustedBoundingBox(m_boundingBox);
	}


	inline int CSFPolyLine::GetNbPoints()const
	{
		return (int)m_points.size();
	}
	inline int CSFPolyLine::GetNbPoints(int ringNo)const
	{
		int first = 0;
		int last = 0;
		GetPointIndex(ringNo, first, last);
		return last - first + 1;
	}



	int CSFPolyLine::NewRing()
	{
		m_beginParts.push_back((int)m_points.size());
		return (int)m_beginParts.size() - 1;
	}

	inline int CSFPolyLine::GetNbRing()const
	{
		return (int)m_beginParts.size();
	}


	inline void CSFPolyLine::GetPointIndex(int ringNo, int& first, int& last)const
	{
		ASSERT(ringNo >= 0 && ringNo < m_beginParts.size());

		first = m_beginParts[ringNo];
		if (ringNo == m_beginParts.size() - 1)
			last = (int)m_points.size() - 1;
		else last = m_beginParts[ringNo + 1] - 1;

		//ASSERT(first<=last);
	}



	class CSFPolygon : public CSFPolyLine
	{
	public:
		CSFPolygon();
		CSFPolygon(const CSFPolygon& poly);
		virtual ~CSFPolygon();
		virtual CSFShape* GetCopy()const;

		virtual void clear();

		CSFPolygon& operator=(const CSFPolygon& poly);
		bool IsValid()const;
		virtual bool IsInside(const CGeoPoint& P)const;
		inline bool IsInside(int ringNo, const CGeoPoint& P)const;
		virtual inline void SetPoint(int index, const CSFPoint& point, bool bAjustRing = false);
		void ComputeInternalElimination(int nbCols, int nbRows);

	protected:

		int GetXRayCount(int no, const CGeoPoint& P)const;
		void Replace(int first, int last, const CSFPoint& pt);


		//pour optimisation de recherche
		bool m_internalElimination;
		double m_cellWidth;
		double m_cellHeight;
		int m_nbCols;
		boost::dynamic_bitset<size_t> m_internalOutsideGrid; //wich cell are outside
		boost::dynamic_bitset<size_t> m_internalInsideGrid; //wich cell are inside
	};

	inline bool CSFPolygon::IsInside(int ringNo, const CGeoPoint& P)const
	{
		ASSERT(ringNo >= 0 && ringNo < GetNbRing());
		return (GetXRayCount(ringNo, P) % 2) != 0;
	}

	inline void CSFPolygon::SetPoint(int index, const CSFPoint& point, bool bAjustRing)
	{
		m_points[index] = point;

		//replace the first and the last point of a ring
		if (bAjustRing)
		{
			int ringIndex = GetRingNo(index);
			int first = 0, last = 0;
			GetPointIndex(ringIndex, first, last);

			if (first < last)
			{
				if (index == first)
					m_points[last] = point;

				if (index == last)
					m_points[first] = point;
			}
		}

		GetAdjustedBoundingBox(m_boundingBox);
	}
	//************************************************************************
	class CSFRecord
	{
	public:

		CSFRecord(int type);
		CSFRecord(const CSFRecord& record);
		~CSFRecord();

		CSFRecord& operator=(const CSFRecord& record);

		template<class Archive>
		Archive& Read(Archive& io)
		{

			if (m_pShape)
			{
				if (m_pShape->GetType() == CShapeFileHeader::SHAPE_NULL)
					((CSFShape*)m_pShape)->Read(io);
				else if (m_pShape->GetType() == CShapeFileHeader::POINT)
					((CSFPoint*)m_pShape)->Read(io);
				else if (m_pShape->GetType() == CShapeFileHeader::POLYLINE)
					((CSFPolyLine*)m_pShape)->Read(io);
				else if (m_pShape->GetType() == CShapeFileHeader::POLYGON)
					((CSFPolygon*)m_pShape)->Read(io);
				else if (m_pShape->GetType() == CShapeFileHeader::MULTIPOINT)
					((CSFMultiPoint*)m_pShape)->Read(io);

			}

			return io;
		}

		template<class Archive>
		Archive& Write(Archive& io)
		{
			WriteBigEndian(io, m_number);
			WriteBigEndian(io, GetLength());

			int type = m_pShape->GetTypeNumber();
			io << type;


			if (m_pShape->GetType() == CShapeFileHeader::SHAPE_NULL)
				((CSFShape*)m_pShape)->Write(io);
			else if (m_pShape->GetType() == CShapeFileHeader::POINT)
				((CSFPoint*)m_pShape)->Write(io);
			else if (m_pShape->GetType() == CShapeFileHeader::POLYLINE)
				((CSFPolyLine*)m_pShape)->Write(io);
			else if (m_pShape->GetType() == CShapeFileHeader::POLYGON)
				((CSFPolygon*)m_pShape)->Write(io);
			else if (m_pShape->GetType() == CShapeFileHeader::MULTIPOINT)
				((CSFMultiPoint*)m_pShape)->Write(io);


			return io;
		}


		CSFShape* GetShape(int type);

		__int32 GetNumber()const{ return m_number; }
		void SetNumber(__int32 number){ m_number = number; }
		inline __int32 GetLength()const;
		//inline void SetLength(__int32 length);
		inline CSFShape& GetShape()const;
		inline void SetShape(const CSFShape& pShape);

		// inline bool Draw(CDC* pDC, const CRect& rcBounds, const CShowViewport& viewPort, CProjection const & projection);
	private:



		__int32 m_number;
		__int32 m_length;
		CSFShape* m_pShape;
	};

	inline CSFShape& CSFRecord::GetShape()const
	{
		return *m_pShape;
	}
	inline void CSFRecord::SetShape(const CSFShape& shape)
	{
		delete m_pShape;
		m_pShape = shape.GetCopy();
	}
	inline __int32 CSFRecord::GetLength()const
	{
		ASSERT(m_pShape);
		return m_pShape->GetLength();
	}
	/*inline void CSFRecord::SetLength(__int32 length)
	{
	m_length = length;
	}
	*/
	/*inline bool Draw(CDC* pDC, const CRect& rcBounds, const CShowViewport& viewPort, CProjection const & projection)
	{
	m_pShape
	}*/

	typedef std::vector<CSFRecord*> CSFRecordArray;
	//**************************************************************
	class CShapeFileBase
	{
	public:

		enum TOpen{ modeRead, modeWrite };

		CShapeFileBase();
		CShapeFileBase(const CShapeFileBase& shapeFile);
		~CShapeFileBase();
		CShapeFileBase& operator=(const CShapeFileBase& shapeFile);
		const CSFRecord& operator[](int index){ return *(m_records[index]); };

		size_t GetPrjID()const{ return m_pPrj ? m_pPrj->GetPrjID() : PRJ_NOT_INIT; }
		const CGeoRect& GetBoundingBox()const{ return m_header.GetBoundingBox(); }
		void SetBoundingBox(const CGeoRect& bounds){ m_header.SetBoundingBox(bounds); }

		ERMsg  Read(const std::string& filePath);
		ERMsg  Write(const std::string& filePath);

		void Create(const CShapeFileBase& shapeFile, const std::set<int>& recordNo);
		ERMsg Create(const CShapeFileBase& shapeFile, const std::string& fieldName, const StringVector& zoneArray);
		ERMsg Create(const CShapeFileBase& shapeFile, int fieldNo, const StringVector& zoneArray);
		ERMsg Create(const CShapeFileBase& shapeFile, int fieldNo, int uniqueID);
		bool TransformeProjection(CProjectionTransformation const& PT);

		void clear();


		static CShapeFileHeader::TShape GetShapeType(const std::string& filePath);
		CShapeFileHeader::TShape GetShapeType()const{ return m_header.GetShapeType(); }
		void SetShapeType(CShapeFileHeader::TShape type);

		int GetNbShape()const{ return (int)m_records.size(); };
		void AddShape(const CSFShape& shape, const CDBFRecord& shapeInfo);
		void AddShape(const CSFShape& shape);
		void RemoveShape(int shapeNo);
		void SetShape(int shapeNo, const CSFShape& shape);

		virtual ERMsg Open(const std::string& filePath, int openMode);
		virtual ERMsg Close(bool bSaveInfoFile = true);
		void GetInformation(std::string& information)const;


		//bool PtInShapeFile(const CGeoPoint& pt, int* pPolyNo = NULL)const;//point geographique
		//bool IsInside(const CGeoPointWP& pt, int* pPolyNo = NULL)const;//dasn la meme projection
		bool IsInside(const CGeoPoint& pt, int* pPolyNo = NULL)const;//dans la meme projection
		//double GetMinimumDistance(const CGeoPointWP& pt, int* pPolyNo = NULL)const;
		double GetMinimumDistance(const CGeoPoint& pt, int* pPolyNo = NULL)const;

		bool SometingInRect(const CGeoRect& rect)const;
		bool IsRectIntersect(const CGeoRect& rect)const;
		void ComputeInternalElimination(int nbCols, int nbRows);

		void GetShapeFileIndex(CShapeFileIndex& shx)const;

		void CheckOrCreateDBF();
		static std::string GetIndexName(const std::string& filePath);
		static std::string GetDBFName(const std::string& filePath);
		static std::string GetPrjFilePath(const std::string& filePath);

		ERMsg WriteDBF(const std::string& filePath)const;
		ERMsg ReadDBF(const std::string& filePath);

		ERMsg WriteIndex(const std::string& filePath)const;


		CDBF3& GetDBF(){ return m_infoDBF; }
		const CDBF3& GetDBF()const { return m_infoDBF; }
		void SetDBF(const CDBF3& newDBF){ m_infoDBF = newDBF; }
		int GetCount()const{ return (int)m_records.size(); }
		CSFRecordArray& GetRecords(){ return m_records; }
		const CSFRecordArray& GetRecords()const{ return m_records; }

		//virtual void SetProjection(CProjection const & projection);
		//virtual CProjection const & GetProjection()const;

		void UpdateHeader();
		void GetFieldArray(StringVector& fields, bool bGetAll = false)const;
		void GetFieldValues(int fieldIndex, StringVector& values)const;

	protected:

		void SynchronizeProjection();



		template<class Archive>
		CSFRecord* GetRecord(Archive& io)
		{
			CSFRecord* pRecord = NULL;
			try
			{
				//if (io.tellp() < streamEnd)
				{
					unsigned __int32 number = (unsigned __int32)ReadBigEndian(io);
					unsigned __int32 length = (unsigned __int32)ReadBigEndian(io);


					if (number > 0 && length > 0)
					{
						__int32 shapeType = 0;
						io >> shapeType;
						ASSERT(shapeType == m_header.GetShapeType() || shapeType == CShapeFileHeader::SHAPE_NULL);

						pRecord = new CSFRecord(shapeType);

						pRecord->SetNumber(number);

						pRecord->Read(io);
					}
				}
			}
			catch (boost::archive::archive_exception e)
			{
				//end of file do nothing
			}

			return pRecord;
		}


		CShapeFileHeader m_header;
		CSFRecordArray   m_records;
		CDBF3			 m_infoDBF;
		CProjectionPtr	m_pPrj;


		std::string m_filePath;

	};

}